

	//dest1 := X1.GetDestination(dest.DId)

	fmt.Println("hello 2", X1.destinations[dest.DId].admf.id)

	// fmt.Println("dest1", x1message.X1RequestMessageList[0].AdmfIdentifier)

	// fmt.Println("dest1", x1message.X1RequestMessageList[0].AdmfIdentifier)

		// "github.com/stretchr/testify/mock"

		
	resp := X1.createDestination(job, &x1message.X1RequestMessageList[0], &x1Response)

	assert.Equal(t, 2030, resp.ErrorCode)

	

	// x1message.X1RequestMessageList[0].DId = "b0ce308c-aa17-42bd-a27b-287bcb5b3469"

	// fmt.Println(x1message.X1RequestMessageList[0].DId)

	// resp = X1.createDestination(job, &x1message.X1RequestMessageList[0], &x1Response)

	// assert.Equal(t, 6000, resp.ErrorCode)

	//assert.Equal(t, x1Parser.X1CreateDestination, resp)

// import (
// 	log "X1If/pkg/mslibs/zaplog"
// 	"testing"
// 	"github.com/stretchr/testify/assert"
// )


consider removing this. See comment in adapters.go

Just a heads up (I do not think this is a problem):
You have (ever so slightly) changed the behavior.
InitRedis() is blocking. It will wait forever until it creates a connections towards redis-server.
That was also the case before, but then we did it inside NewX1Server()


func TestCreateDestination(t *testing.T) {
	c := config.NewConfiguration()
	db := &TargetDbMock{}
	X1 = NewX1Server(c, db)
	x1Response := x1enc.X1ResponseMessage{
		AdmfIdentifier:  "Admf",
		NeIdentifier:    "neid",
		Version:         "v1.7.1",
		X1TransactionId: "12",
	}
	job := &mscore.Job{}
	var x1message x1dec.X1Request
	xml.Unmarshal([]byte(xmltestpatterns.CreateDestinationRequest), &x1message)

	X1.createDestination(job, &x1message.X1RequestMessageList[0], &x1Response)

	assert.Equal(t, "AcknowledgedAndCompleted", string(x1Response.OK))
}

func TestModifyDestination(t *testing.T) {

	c := config.NewConfiguration()
	db := &TargetDbMock{}
	X1 = NewX1Server(c, db)
	x1Response := x1enc.X1ResponseMessage{
		AdmfIdentifier:  "Admf",
		NeIdentifier:    "neid",
		Version:         "v1.7.1",
		X1TransactionId: "12",
	}
	job := &mscore.Job{}
	var x1message x1dec.X1Request
	xml.Unmarshal([]byte(xmltestpatterns.CreateDestinationRequest), &x1message)
	req := x1message.X1RequestMessageList[0]
	dest := req.DestinationDetails

	X1.destinations[dest.DId] = &Destination{
		detail: &x1enc.DestinationDetails{
			DId:             dest.DId,
			FriendlyName:    dest.FriendlyName,
			DeliveryType:    x1ie.DeliveryType(dest.DeliveryType.String()),
			DeliveryAddress: x1enc.CopyDeliveryAddress(dest.DeliveryAddress),
		},
		admf: &ADMF{
			id:   req.AdmfIdentifier,
			addr: req.DestinationDetails.DeliveryAddress.Uri,
		},
	}
	dest.DeliveryType = x1ie.DeliveryType("X2Only")

	X1.modifyDestination(job, &req, &x1Response)

	assert.Equal(t, "AcknowledgedAndCompleted", string(x1Response.OK))
}


type KEYEVENT string

type Duration int64

type baseCmd struct {
	_args []interface{}
	err   error

	_readTimeout *Duration
}

type StatusCmd struct {
	baseCmd

	val string
}



type KEYEVENT string

type Duration int64

type baseCmd struct {
	_args []interface{}
	err   error

	_readTimeout *Duration
}

type StatusCmd struct {
	baseCmd

	val string
}


I need to put a commit name where i created interface and now I inject instance to created interfacce ?

"X1If/internal/app/x1"

targetDb
// package X1If

// import (
// 	"X1If/internal/pkg/interfaces"
// 	"X1If/internal/pkg/redisadapter"

// 	"github.com/go-redis/redis"
// )

// type TargetDbMock struct {
// }

// func (*TargetDbMock) ConfigSet(parameter string, value string) *redis.StatusCmd {
// 	return nil
// }

// func NewStatusCmd(s1, s2, parameter, value string) {
// }

// // ConnectToDb implements TargetDb.
// func (*TargetDbMock) ConnectToDb(net string, ep string) error {
// 	return nil
// }

// // FlushAll implements TargetDb.
// func (*TargetDbMock) FlushAll() {
// }

// // GetAllTasks implements TargetDb.
// func (*TargetDbMock) GetAllTasks(admfID string, params interfaces.ParamContext, taskToXML func(interface{}, interfaces.ParamContext, bool)) {
// }

// // IsAlive implements TargetDb.
// func (*TargetDbMock) IsAlive() bool {
// 	return true
// }

// // IsMaster implements TargetDb.
// func (*TargetDbMock) IsMaster() (bool, error) {
// 	return true, nil
// }

// // ReadTaskDetails implements TargetDb.
// func (*TargetDbMock) ReadTaskDetails(admfID string, taskID string, admfDependant bool, admfIdList []string) (task map[string]string, x2 map[string]string, fseids []string, locked bool, err error) {
// 	task = make(map[string]string)
// 	x2 = make(map[string]string)
// 	fseids = make([]string, 1)
// 	return task, x2, fseids, true, nil
// }

// // SubscribeToKeyEvent implements TargetDb.
// func (*TargetDbMock) SubscribeToKeyEvent(key string, event redisadapter.KEYEVENT) (chan bool, error) {
// 	return make(chan bool), nil
// }

// func (targetDB *TargetDbMock) CreateDestination(destinationID string,
// 	admfID string,
// 	neID string,
// 	msgTime string,
// 	version string,
// 	friendlyName string,
// 	deliveryType string,
// 	ipEndpoint string,
// 	e164Number string,
// 	uri string,
// 	email string,
// 	destinationDetailsExtension string) error {
// 	return nil
// }

// func (targetDB *TargetDbMock) ModifyDestination(destinationID string,
// 	admfID string,
// 	neID string,
// 	msgTime string,
// 	version string,
// 	friendlyName string,
// 	deliveryType string,
// 	ipEndpoint string,
// 	e164Number string,
// 	uri string,
// 	email string,
// 	destinationDetailsExtension string) (int64, error) {
// 	return 1, nil
// }

// func (targetDB *TargetDbMock) RemoveDestination(admfID string, destinationID string) (int64, error) {
// 	return 1, nil
// }

// func (targetDB *TargetDbMock) GetDestinationDetails(dID string) (map[string]string, error) {
// 	return make(map[string]string), nil
// }

// func (targetDB *TargetDbMock) ActivateTask(taskID string,
// 	admfID string,
// 	neID string,
// 	msgTime string,
// 	version string,
// 	transactID string,
// 	targetIdentifierList []string,
// 	deliveryType string,
// 	destinationList []string,
// 	mediationList string,
// 	correlationID string,
// 	implicitDeactivation string,
// 	productID string,
// 	taskDetailsExtension string,
// 	targetIdentifierListK2Hashed []string,
// 	k2Key string) (int64, error) {
// 	return 1, nil
// }

// func (targetDB *TargetDbMock) ModifyTask(taskID string,
// 	admfID string,
// 	neID string,
// 	msgTime string,
// 	version string,
// 	targetIdentifierList []string,
// 	deliveryType string,
// 	destinationList []string,
// 	mediationList string,
// 	correlationID string,
// 	implicitDeactivation string,
// 	productID string,
// 	taskDetailsExtension string,
// 	admfDependant bool,
// 	admfIdList []string) (int64, error) {
// 	return 1, nil
// }

// func (targetDB *TargetDbMock) DeactivateTask(admfID string, taskID string, admfDependant bool, admfIdList []string) (int64, error) {
// 	return 1, nil
// }

// func (targetDB *TargetDbMock) GetTaskDetails(admfID string,
// 	taskID string,
// 	admfDependant bool,
// 	admfIdList []string) (task map[string]string, x2 map[string]string, fseids []string, locked bool, err error) {
// 	task = make(map[string]string)
// 	x2 = make(map[string]string)
// 	fseids = make([]string, 1)
// 	return task, x2, fseids, true, nil
// }

// func (targetDB *TargetDbMock) GetDestinationKeys() ([]string, error) {
// 	return make([]string, 0), nil
// }
// func (targetDB *TargetDbMock) GetDestinationDetail(key string) (map[string]string, error) {
// 	return make(map[string]string), nil
// }

// func (targetDB *TargetDbMock) GetNEStatus(key string, field string) (int64, error) {
// 	return 1, nil
// }

// func (targetDB *TargetDbMock) ReadT3TaskSession(admfID string, taskID string, fseid string) (map[string]string, error) {
// 	return make(map[string]string), nil
// }

// func (targetDB *TargetDbMock) ReadRequiredFSEIDs(admfID string, taskID string, timeToExpire int64) ([]string, error) {
// 	return make([]string, 0), nil
// }

// func (targetDB *TargetDbMock) ReadDestinationStatus(dID string) (map[string]string, error) {
// 	return make(map[string]string), nil
// }

// func (targetDB *TargetDbMock) ListAllXIDs(admfID string, admfDependant bool) ([]string, error) {
// 	return make([]string, 0), nil
// }

// func (targetDB *TargetDbMock) ReadUserInfo(userID string, tid string) (map[string]string, error) {
// 	return make(map[string]string), nil
// }

// func (targetDB *TargetDbMock) ClearUserInfo(userID string, tid string) error {
// 	return nil
// }

// func (targetDB *TargetDbMock) ReadX1Config() (map[string]string, error) {
// 	return make(map[string]string), nil
// }

// func (targetDB *TargetDbMock) StepCountersAsync(name string, step int) {
// }

// func (targetDB *TargetDbMock) StepCounterOfADMF(admf, name string, step int) {
// }

// func (targetDB *TargetDbMock) SubscribeToConfigChange() (chan bool, error) {
// 	return make(chan bool), nil
// }

// func (targetDB *TargetDbMock) SubscribeToNewIssues() (chan bool, error) {
// 	return make(chan bool), nil
// }

// func (targetDB *TargetDbMock) SubscribeToAddDest() (chan bool, error) {
// 	return make(chan bool), nil
// }

// func (targetDB *TargetDbMock) SubscribeToRemDest() (chan bool, error) {
// 	return make(chan bool), nil
// }

// func (targetDB *TargetDbMock) SendAlarm(alarm string) error {
// 	return nil
// }

// func (targetDB *TargetDbMock) Publish(channel string, message interface{}) error {
// 	return nil
// }

// func (targetDB *TargetDbMock) RPush(name string, values ...interface{}) error {
// 	return nil
// }

// func (targetDB *TargetDbMock) HSet(hash string, field string, value interface{}) error {
// 	return nil
// }

// func (targetDB *TargetDbMock) HGetAll(hash string) (map[string]string, error) {
// 	return make(map[string]string), nil
// }

// func (targetDB *TargetDbMock) HGet(hash, field string) (string, error) {
// 	return "", nil
// }

// func (targetDB *TargetDbMock) SAdd(hash string, values ...interface{}) error {
// 	return nil
// }

// func (targetDB *TargetDbMock) SRem(hash string, values ...interface{}) error {
// 	return nil
// }

// func (targetDB *TargetDbMock) Del(keys ...string) error {
// 	return nil
// }

// func (targetDB *TargetDbMock) Exists(keys string) bool {
// 	return true
// }

// func (targetDB *TargetDbMock) PollingIssues() ([]interface{}, bool, error) {
// 	return make([]interface{}, 1), true, nil
// }

// func (targetDB *TargetDbMock) ListAllXidsForAdmf(admfID string) []string {
// 	return make([]string, 0)
// }

// func (targetDB *TargetDbMock) ListAllDidsForAdmf(admfID string) []string {
// 	return make([]string, 0)
// }

// func (targetDB *TargetDbMock) GetAllDetails(admfId string) ([]string, []map[string]string, []map[string]string, error) {
// 	finalTaskSliceMaps := []map[string]string{}
// 	return make([]string, 0), finalTaskSliceMaps, finalTaskSliceMaps, nil
// }

// func (targetDB *TargetDbMock) GetAllDestinations(admfID string, params interfaces.ParamContext, destinationToXML func(interface{}, interfaces.ParamContext, bool)) {
// }


modified:   internal/app/x1/interface.go
modified:   internal/pkg/interfaces/redisadapter.go
modified:   internal/pkg/redisadapter/adapters.go
modified:   internal/pkg/redisadapter/redisadapter.go



func RedisSocketType(c *config.CONFIGURATION) ConnectionConfig {
	var redisSocket ConnectionConfig
	redisSocket.redisSockType = c.GetString("redisSocketType", "tcp")

	if redisSocket.redisSockType == "" || redisSocket.redisSockType == "tcp" {
		redisSocket.redisSockType = "tcp"
		redisSocket.ep = c.GetString("redisHost", "localhost") + ":" + strconv.Itoa(c.GetInt("redisPort", 6379))
	} else { //unix socket
		redisSocket.ep = c.GetString("redisUnixSocket", "")
		if len(redisSocket.ep) == 0 || redisSocket.redisSockType != "unix" {
			log.Fatal("No valid socket type or file descriptor is provided for redis connection.")
		}
	}
	return redisSocket
}

// InitRedis try to connect to the DB
func InitRedis(redisSocket ConnectionConfig) *RedisAdapter {
	targetDb := CreateRedisAdapter()
	log.Debugf("Connecting to Redis @[%s] %s", redisSocket.redisSockType, redisSocket.ep)
	err := targetDb.ConnectToDb(redisSocket.redisSockType, redisSocket.ep)
	if err != nil {
		log.Warnf("Unable to connect to Redis @[%s] %s, will try later...", redisSocket.redisSockType, redisSocket.ep)
	}
	return targetDb
}


Hello,




if the TR PCTR-77719 is behind this or not isn't known at the moment, 
but it is a tr correction and a correction will come soon for that actual issue. 
The rest will remain for UP to answer, especially of 2 DTLS tunnels is normal/can happen occasionally or if it may be due to this PCTR-77719.

Br,

Sai.

Hello, I might miss the standup today as I will be travelling. I will work for a few hours in the morning to wrap up my tasks.
Regarding go task, I have submitted the target DB interface commit, and another one is currently under review, for which I have already addressed the received comments. Can anyone please submit it if it is given +2? and, I have written a few unit tests for dest.go by implementing the targetdb mock which got some comments. I am not sure if I can fix it or not before my vacation. There are some go files in x1 package that need to be tested through targetDb mock. Feel free to continue working on unit tests if anyone is interested.
Regarding the service desk ticket, I have updated the information that If the TR PCTR-77719 is behind this or not isn't known at the moment, but it is a TR  and a correction will come soon for that actual issue. The rest will remain for UP to answer, especially of 2 DTLS tunnels is normal/can happen occasionally or if it may be due to this PCTR-77719. I will check with the reporter to see if I can close it or not.
has context menu




