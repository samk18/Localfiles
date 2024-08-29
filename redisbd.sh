#!/usr/bin/env bash

#set -x

source /opt/services/epg/libexec/sh/ipos.sh # IPOS specific functions like card_type, etc

RUN_DIR="/var/run/redis"
LOG_DIR="/md/redis"
BIN_DIR="/opt/services/epg/bin"
ETC_DIR="/opt/services/epg/etc"
REDIS_CONFIG_FILE=${ETC_DIR}/redis/redis_li.conf
REDIS_SERVER=${BIN_DIR}/redis-server
REDIS_CLI=${BIN_DIR}/redis-cli
LOG_FILE=${LOG_DIR}/redis_db.log # This LOGFILE is used before execing the redis process
REDIS_PORT_BASE=6000


get_redis_args()
{
# dir
    local localProcessId=$1
    local role=$2

    local redis_port=$(get_redis_port_number $localProcessId $role)
    local host_address=$(get_host_address)
    local logfile=${LOG_DIR}/redis_db_${redis_port}.log
    local pidfile=${RUN_DIR}/redis_${redis_port}.pid
    local dir=${RUN_DIR}/redis_${redis_port}
    mkdir -p "$dir"
    [ -d "$dir" ] || print_log "ERROR: failed to create $dir"
    local ARGS=$(echo " --pidfile $pidfile --logfile $logfile --dir $dir --bind $host_address 127.0.0.1 --port $redis_port --unixsocket ${dir}/redis.sock --unixsocketperm 755 --notify-keyspace-events KEA --repl-diskless-sync yes --repl-diskless-sync-delay 0")
    if [ "$role" == "slave" ]; then # Check if this is supposed to be a slave
	ARGS+=" --slaveof ${host_address} $REDIS_PORT_BASE "
    fi
    echo $ARGS
}

is_gsc_board()
{
    local rt="false"
    local gsc_process_num=`ps -ef | grep -E '([a-z]+/)+pgwcd.+ .+ .+gsc' | wc -l`
    [ $gsc_process_num == 0 ] && rt="false" || rt="true"
    echo "$rt"
}

select_standby()
{
    local maxOffset=0
    local slaveOffset=0
    local ports=`ls /var/run/redis/redis_*.pid | grep -oE '[0-9]+' | grep -v ${REDIS_PORT_BASE}`
    local candidate=0
    for port in $ports
    do
        slaveOffset=`${REDIS_CLI} -p ${port} info replication|grep slave_repl_offset | grep -oE '[0-9]+'`
        [ $slaveOffset -gt $maxOffset ] && maxOffset=$slaveOffset && candidate=$port
    done
    echo $candidate
}

set_standby_slaveof_no_one()
{
    rt=`${REDIS_CLI} -p $1 slaveof no one`
    print_log "${REDIS_CLI} -p $1 slaveof no one"
    print_log "set_standby_slaveof_no_one: $rt"
}

reset_standby_slaveof_master_once_sync_done()
{
    local host=$(get_host_address)
    local port_m=$REDIS_PORT_BASE
    local port_s=$1
    ${BIN_DIR}/redis_db_recover ${host} ${port_m} ${port_s} &
}

get_args_slaveof_standby()
{
    local host_address=$(get_host_address)
    echo "--slaveof ${host_address} $1"
}

get_board_name()
{
    physicalProcessId=$1 # board016:redis_db:2
    echo $physicalProcessId | awk -F":" '{print $1}'
}

get_instance()
{
    physicalId=$1 #board016:redis_db:2
    echo $physicalId | awk -F":" '{print $3}'
}

get_local_instance()
{
    physicalId=$1 #board016:redis_db:2
    local numberOfInstances=$2
    local tmpId=$(get_instance $physicalId)
    tmpId=$(( $tmpId % $numberOfInstances))
    echo $(($tmpId))
}


# Get port number from  local instance number
get_redis_port_number()
{
    local localnstanceId=$1
    local role=$2

    if [ "$role" == "master" ]; then
        echo $REDIS_PORT_BASE
    else
        port=$(($REDIS_PORT_BASE+$localnstanceId+1))
        echo $port
    fi
}

create_directories()
{
    [ -d $LOG_DIR ] || mkdir -p  $LOG_DIR
    [ -d /flash/services/epg/redis/ ] || mkdir -p /flash/services/epg/redis/
    [ -d $RUN_DIR ] || mkdir -p  $RUN_DIR
    # if there's no default cfg, copy it from package
    # cp /opt/services/epg/bin/ttx_redis_default.cfg /flash/services/epg/redis/
}

self="$0"
print_log() {
    log_message="$*"
    log_date=`date "+%F %H:%M:%S"`
    echo "$log_date [$self] $log_message" >> $LOG_FILE
}

rotate_log() {
    # 1Mb rotate
    if [ -f $LOG_FILE ]; then
    IS_ROTATE=$(ls -l $LOG_FILE | awk '{if ($5 > 1024000) {print "rotate"} else {print "no"}}')
        if [ "rotate" = "$IS_ROTATE" ]; then
            mv -f $LOG_FILE ${LOG_FILE}.0
            print_log "redis log rotated"
        fi
    fi
}

# This only works on the RP
get_cpb_addresses()
{
    address_list=""
    while read  board address; do
        if [[ "$board" = "gc-"* ]] ; then
        address_list+=" $address"
    fi
    done < <( /opt/services/epg/bin/ttx/bin/epg_pdc_run_command  "ManagedElement=1,Epg=1,Node=1,status" | awk '/internal-address:/ || /board:/ {print }' | tr -d "\r"  |  sed -e 's/board://g' -e 's/internal-address://g' | while read line1; do read line2; echo "$line1 $line2"; done )

    echo $address_list
}

get_host_address()
{
    ip -4 addr show dev ethSw0|awk '/^\s*inet\s.*ethSw0$/{sub("/.*","",$2); print $2}'
}

# Ignore following signals
#trap "" INT PIPE STOP HUP
trap "echo got signal $*" INT PIPE STOP HUP

usage() { echo "Usage: $0 -r [ master | slave ]" 1>&2; exit 1; }

main()
{

    # Args typically
    # board016:redis_db:2 [ProcessColdStart|ProcessRestart]


    ROLE=undefined
    NBINSTANCE=1

    while getopts ":p:r:s:" option; do
	case "${option}" in
            p)
		NBINSTANCE=${OPTARG}
		;;
            r)
		ROLE=${OPTARG}
		if [ "${ROLE}" != "master" ] && [  "${ROLE}" != "slave" ]; then
		    usage
		fi
		;;
            s)
		INSTANCE=${OPTARG}
		;;

            *)
		;;
	esac
    done
    shift $((OPTIND-1))

    echo "Left args $*"

    echo "role = ${ROLE}"

    physicalProcessId=$1
    restartLevel=$2

    local standbyPort=0
    extra_args=""
    if [ "${restartLevel}" == "ProcessRestart" ] && [ "${ROLE}" == "master" ]; then
        is_gsc_cpb=$(is_gsc_board)
        if [ "${is_gsc_cpb}" == true ]; then
            print_log "Master DB restarts on GSC Board."
            standbyPort=$(select_standby)
            if [ $standbyPort != 0 ]; then
                extra_args=$(get_args_slaveof_standby $standbyPort)
                $(set_standby_slaveof_no_one $standbyPort)
                $(reset_standby_slaveof_master_once_sync_done $standbyPort) &
            else
                print_log "Master DB restarts, but Standby DB cannot be found."
            fi
        else
            print_log "Master DB restarts on PSC Board."
        fi
    else
        print_log "Not master, not restart or not on GSC board."
    fi

    local localProcessId=$(get_local_instance $physicalProcessId $NBINSTANCE)

    create_directories
    local redis_args=$(get_redis_args $localProcessId $ROLE)
    print_log "ROLE: $ROLE NBINSTANCE: $NBINSTANCE INSTANCE: $INSTANCE physicalProcessId: $physicalProcessId restartLevel: $restartLevel"
    print_log "Starting redis with arguments: $redis_args"
    echo "NBINSTANCE="$NBINSTANCE > ${RUN_DIR}/redis_instances.cfg

    print_log "--------   redis_db will be replaced by the following redis_server"
    print_log "exec -a redis_l${ROLE}${localProcessId} $REDIS_SERVER $REDIS_CONFIG_FILE $redis_args $extra_args"
    exec -a "redis_l${ROLE}${localProcessId}" $REDIS_SERVER $REDIS_CONFIG_FILE $redis_args $extra_args

}


print_log "-------->   redis_db start"
print_log "all arguments: "$*

main $*
