// Copyright Ericsson AB 2000-2023. All Rights Reserved.

#include "PsciSftFramework/Amf/Amf.h"
#include "PsciSftFramework/Gsc/Gsc.h"
#include "PsciSftFramework/Li/Df2.h"
#include "PsciSftFramework/Li/EtsiDf2.h"
#include "PsciSftFramework/Li/LiT3.h"
#include "PsciSftFramework/N7/GxN7Converter.h"
#include "PsciSftFramework/Nrf/Nrf.h"
#include "PsciSftFramework/Nrf/SftNrfd.h"
#include "PsciSftFramework/Pcf/Pcf.h"
#include "PsciSftFramework/Pfcp/FarHelper.h"
#include "PsciSftFramework/Procedure/ProcedureData.h"
#include "PsciSftFramework/Procedure/PscProcedure.h"
#include "PsciSftFramework/SftEtsiLi.h"
#include "PsciSftFramework/SftLi.h"
#include "PsciSftFramework/SftSutPgw.h"
#include "PsciSftFramework/SftUe.h"
#include "PsciSftFramework/Sgw/Sgw.h"
#include "PsciSftFramework/Udm/Udm.h"
#include "PsciSftFramework/Upf/Upf.h"
#include "PsciSftFramework/Upf/UpfMessageFactory.h"
#include "PsciSftHelpers/Bearer/Bearer.h"
#include "PsciSftHelpers/TestFixtures/smf/predefinedRules/PredefinedRulesTestFixtureDeprecated.h"
#include "PsciSftHelpers/Util/MultipleQosFlows.h"
#include "PsciSftHelpers/procedures/AnRelease.h"
#include "PsciSftHelpers/procedures/CreateQosFlows.h"
#include "PsciSftHelpers/procedures/DeleteBearer.h"
#include "PsciSftHelpers/procedures/DeleteQosFlows.h"
#include "PsciSftHelpers/procedures/Eps5gs.h"
#include "PsciSftHelpers/procedures/Establish5gSession.h"
#include "PsciSftHelpers/procedures/Mobility5g4g.h"
#include "PsciSftHelpers/procedures/N2Handover.h"
#include "PsciSftHelpers/procedures/Release5gSession.h"
#include "PsciSftHelpers/procedures/ServiceRequest.h"
#include "SftGx/BasicChargingRules.h"
#include "SftGx/PredefinedPccRule.h"
#include "SftN11/Message.h"
#include "SftN11/MessageBuilder.h"
#include "SftN40/Message.h"
#include "SftN40/MessageBuilder.h"
#include "SftPgwcd/Bearer.h"
#include "SftPgwcd/Li/Redis.h"
#include "SftPgwcd/Li/RedisMessage.h"
#include "SftPgwcd/Li/SessionTasks.h"
#include "SftPgwcd/Session.h"
#include "SftProtocolCommon/Util/VerificationLevel.h"
#include "TestSupportConfiguration/Builder/Builder.h"
#include "TestSupportConfiguration/Builder/FeatureActivation.h"
#include "TestSupportConfiguration/Builder/LawfulInterceptEtsiReporting.h"
#include "TestSupportConfiguration/Builder/NodeFeatureActivation.h"
#include "TestSupportConfiguration/Builder/Pgw.h"
#include "TestSupportConfiguration/Builder/PgwLawfulIntercept.h"
#include "TestSupportConfiguration/Configuration.h"
#include "TestSupportConfiguration/CurrentConfiguration.h"
#include "TestSupportGtpV2/Gtpv2Message.h"
#include "TestSupportJson/Json.h"
#include "TestSupportLiConfiguration/LiConfiguration.h"
#include "TestSupportN2/PathSwitchRequestTransfer.h"
#include "TestSupportSftComponents/Lcmd/ConfigurationUtil.h"
#include "TestSupportSftCore/Banner.h"
#include "TestSupportSftLogger/Logger.h"
#include "TestSupportTypes/BearerQos.h"
#include "TestSupportTypes/FlowDescription.h"
#include "TestSupportTypes/JsonSerializable.h"
#include "TestSupportTypes/Qci.h"
#include "TestSupportTypes/Supi.h"
#include "TestSupportX2/X2Message.h"

#include <iterator>

#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

using namespace Compare;
using namespace Sft::Li;
using namespace Sft::N7;
using namespace Sft::pfcp;
using namespace Sft;
using namespace std::chrono_literals;
using namespace TestSupport::Types::literals;
using namespace TestSupport::Types;

class LawfulInterceptionEtsiMultipleQosFlow : public SftSutSmf,
                                              public SftUpf,
                                              public SftUe,
                                              public SftEtsiLi,
                                              public SftLi,
                                              public SftPcf,
                                              public SftNrfd,
                                              public SftUdm,
                                              public SftSgw,
                                              public SftAmf
{
public:
    LawfulInterceptionEtsiMultipleQosFlow()
        : LawfulInterceptionEtsiMultipleQosFlow(Sft::Li::SessionTasks::Destination2.ipendpoint_)
    {
    }

    explicit LawfulInterceptionEtsiMultipleQosFlow(const TestSupport::Types::Endpoint& df2Endpoint)
        : SftEtsiLi(df2Endpoint)
    {
        // Disable auto reply so that the UE can be tracked
        liRedis_.getAutoReplier().disable(Sft::Li::RedisMessage::Type::TargetInfoRequest);
    }

    void create5gSession(const Li::Task& task)
    {
        Procedures::establish5gSession.runActionsBefore("redisReceiveTargetInfoRequest");
        liRedis_.receive(session_, liRedis_.targetInfoRequest(session_));
        liRedis_.send(liRedis_.targetInfoEtsiReply(session_, {task}));
        Procedures::establish5gSession.runActionsBetween("redisSendTargetInfoResponse",
                                                         "Df2ReceivePduSessionEstablishmentRequest");
    }

    void addTaskToRedis(const Li::Task& task)
    {
        liRedis_.sendToSubscribers(liRedis_.onKeySetNotification("task", task.iKey_));
        liRedis_.receive(liRedis_.getAllRequest("task", task.iKey_));
        liRedis_.send(liRedis_.getAllTaskReply(session_, {task}));
    }

    void verifyLiT3(const Sft::Li::Task& task)
    {
        auto lawfulInterceptionRequestActivate = lit3_.lawfulInterceptionRequest_Etsi_Basic(session_, {task})
                                                     .setOperation(Pfcp::Operation::OperationEnum::Set);
        lit3_.receive(session_, lawfulInterceptionRequestActivate);

        lit3_.send(session_, lit3_.lawfulInterceptionResponse(session_));
    }

    Sft::Bearer& installDedicatedBearerinEps()
    {
        Sft::Bearer& newDedicatedBearer = session_.createNewBearer();
        sgw_.receive(session_, sgw_.createBearerRequest({&newDedicatedBearer})
                                   .addToGroup<gtpv2::ie::BearerContext, gtpv2::ie::Pco>(gtpv2::AnyValue));

        sgw_.send(sgw_.createBearerResponse({&newDedicatedBearer}));

        upf_.receive(session_,
                     upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));

        upf_.send(session_, upf_.sessionModificationResponse(session_));
        return newDedicatedBearer;
    }

    void create5gSessionWithPcf(const Li::Task& task)
    {
        Procedures::establish5gSession.runActionsBefore("redisReceiveTargetInfoRequest");
        liRedis_.receive(session_, liRedis_.targetInfoRequest(session_));
        liRedis_.send(liRedis_.targetInfoEtsiReply(session_, {task}));
        Procedures::establish5gSession.runActionsBetween("redisSendTargetInfoResponse", "PcfSendPolicyCreateResponse");
        pcf_.send(session_,
                  N7::MessageBuilder{pcf_.policyCreateResponse(session_, TestSupport::StatusCode::Created)}
                      .addPolicyControlRequestTrigger(N7::PolicyControlRequestTrigger::ServingAreaLocationChange));

        Procedures::establish5gSession.runActionsBetween("PcfSendPolicyCreateResponse",
                                                         "Df2ReceivePduSessionEstablishmentRequest");
    }

    void preTestCase() override
    {
        // ========================
        // === Task added to DB ===
        // ========================
        addTaskToRedis(Li::SessionTasks::Task6);

        SftCore::Logger::banner("Pdu Session establishment");

        create5gSession(Li::SessionTasks::Task6);

        verifyLiT3(Li::SessionTasks::Task6);
        etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
        etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));


        // Create the non-GBR dedicated flow
        constexpr auto numFlows = 1;
        const auto [definitions, bearerQoses] =
            Sft::Util::createDefinitionsAndQoses(Sft::Util::createNonGbrChargingRules(numFlows));
        optionalData        = ProcedureData{definitions, bearerQoses, Li::SessionTasks::Task6};
        dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);

        banner("createQosFlows");
        Procedures::createQosFlows(optionalData, {session_, dedicatedQosBearers});
        const auto qosRules = Sft::Util::createQosRuleDefinitions(
            optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::CreateNewQosRule);
        etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(
            session_, dedicatedQosBearers, qosRules, Sft::Li::QosFlowInfo::FullInfo));
        lit3_.dontReceive();
    }

    void addInitialConfiguration(TestSupport::Configuration::Builder& configuration) override
    {
        // Enable SMF node
        configuration.getNode().getFeatureActivation().setSmf5gc();
        configuration.getPgw()
            .getPgwLawfulIntercept()
            .setEtsiConfig(true)
            .setLiStatusEnabled(true)
            .setLicense(TestSupport::Configuration::PgwLawfulIntercept::LicenseMode::Base)
            .setLawfulInterceptEtsiReporting(
                TestSupport::Configuration::LawfulInterceptEtsiReporting().setSmfEtsiReportingEnabled(true));
        configuration.getPgw().getApn("apn1.com").getInstallPdrMethod().getDedicated().setQci("1-64");
    }

    std::vector<Bearer*> createQosFlows(const ProcedureData& data = {})
    {
        std::vector<Bearer*> bearers = Sft::Util::createQosFlowBearers(data, session_);

        Procedures::createQosFlows(data, {session_, bearers}, {amf_, pcf_, upf_});
        return bearers;
    }

    void postTestCase() override
    {
        lit3_.dontReceive();

        // =======================
        // === Session Release ===
        // =======================
        SftCore::Logger::banner("Pdu Session Release");
        Procedures::release5gSession();
        etsiDf2_.receive(etsiDf2_.pduSessionReleaseInterceptEvent(session_));
        lit3_.dontReceive();
    }

protected:
    ProcedureData        optionalData;
    BasicChargingRules   chargingRules_;
    std::vector<Bearer*> dedicatedQosBearers;
};


//*=============================================================================
// test case: TC79214_NetworkTriggeredServiceRequestCMIdle
//
//#Description#
//    Verify that SMF sends ETSI PDU Session Modification Request for
//    Network Triggered Service Request when CM Idle.
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlow, TC79214_NetworkTriggeredServiceRequestCMIdle)
{
    banner("anRelease");
    Procedures::anRelease(optionalData, {session_, dedicatedQosBearers});

    Procedures::networkTriggeredServiceRequestConnectionManagementIdle(optionalData, {session_, dedicatedQosBearers});
    std::vector<Bearer*> associatedQosFlowList = {};
    associatedQosFlowList.push_back(&session_.getDefaultBearer());
    std::copy(dedicatedQosBearers.begin(), dedicatedQosBearers.end(), back_inserter(associatedQosFlowList));
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosInActiveList(session_, associatedQosFlowList));
}

//*=============================================================================
// test case: TC80506_UETriggeredServiceRequest
//
//#Description#
//     Verify that SMF sends an ETSI PDU Session Modification Request
//     for a UE Triggered Service Request with all successful QoS flows.
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlow, TC80506_UETriggeredServiceRequest)
{
    banner("ueTriggeredServiceRequestMultipleQos");
    Procedures::ueTriggeredServiceRequest(optionalData, {session_, dedicatedQosBearers});
    std::vector<Bearer*> associatedQosFlowList = {};
    associatedQosFlowList.push_back(&session_.getDefaultBearer());
    std::copy(dedicatedQosBearers.begin(), dedicatedQosBearers.end(), back_inserter(associatedQosFlowList));
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosInActiveList(session_, associatedQosFlowList));
}


class LawfulInterceptionEtsiMultipleQosFlowWithAnReleaseWithGbr
    : public LawfulInterceptionEtsiMultipleQosFlow,
      public ::testing::WithParamInterface<N2::RadioNetworkCause>
{
    void preTestCase() override {}
    void postTestCase() override {}
};

INSTANTIATE_TEST_SUITE_P(, LawfulInterceptionEtsiMultipleQosFlowWithAnReleaseWithGbr,
                         ::testing::Values(N2::RadioNetworkCause::UserInactivity, N2::RadioNetworkCause::Redirection,
                                           N2::RadioNetworkCause::RadioConnectionWithUeLost));
// NOLINTBEGIN(cppcoreguidelines-special-member-functions)
//*=============================================================================
// test case: TC79216_ANReleaseWithGBRsPreserveOrDelete
//
//#Description#
//    To verify AN Release with Dedicated GBR QoS Flow which
//    preserves the QoS Flows when the 'UserInactivity' or 'Redirection' happens
//    and deletes when other cause arises.
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_P(LawfulInterceptionEtsiMultipleQosFlowWithAnReleaseWithGbr, TC79216_ANReleaseWithGBRsPreserveOrDelete)
{
    const auto ngApCauseValue = GetParam();
    const auto task           = Li::SessionTasks::Task6;

    // ========================
    // === Task added to DB ===
    // ========================
    addTaskToRedis(task);

    SftCore::Logger::banner("PDU Session Establishment");
    create5gSession(task);

    verifyLiT3(task);
    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));

    // Create the GBR dedicated flow
    auto chargingRuleDefinition = BasicChargingRules().getGbrChargingRules()[0].definition_;
    auto bearerQoses            = Qos::generateBearerQos({chargingRuleDefinition});

    const ProcedureData optionalData{
        chargingRuleDefinition,
        bearerQoses,
        task,
        ngApCauseValue,
        TestSupport::Types::Qci(session_.getDefaultBearer().qos_.qci_),
        TestSupport::Types::Arp(session_.getDefaultBearer().qos_.arp_),
    };

    const auto& dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);

    banner("Create Dedicated QoS Flows");
    Procedures::createQosFlows(optionalData, {session_, dedicatedQosBearers});
    const auto qosRules = Sft::Util::createQosRuleDefinitions(
        optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::CreateNewQosRule);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(session_, dedicatedQosBearers, qosRules,
                                                                               Sft::Li::QosFlowInfo::FullInfo));
    lit3_.dontReceive();

    banner("AN Release");
    Procedures::anRelease(optionalData, {session_, dedicatedQosBearers});

    // UserInactivity and Redirection GBR QoS is being preserved
    if (ngApCauseValue != N2::RadioNetworkCause::UserInactivity && ngApCauseValue != N2::RadioNetworkCause::Redirection)
    {
        // For all other causes the GBR QoS should be removed
        banner("Delete multiple dedicated QoS Flows");
        Procedures::smfDeleteQosFlowsWithUpDeactivatedDlAndUlSeparated(optionalData, {session_, dedicatedQosBearers});
        const auto deleteQosRules = Sft::Util::createQosRuleDefinitions(
            optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::DeleteExistingQosRule);
        etsiDf2_.receive(
            etsiDf2_.pduSessionModificationInterceptEventDeleteQosFlow(session_, dedicatedQosBearers, deleteQosRules));
    }

    banner("Release 5g Session");
    Procedures::release5gSessionWithUpDeactivated();
    etsiDf2_.receive(etsiDf2_.pduSessionReleaseInterceptEvent(session_));
}
// NOLINTEND(cppcoreguidelines-special-member-functions)

//*=============================================================================
// test case: TC79364_NetworkTriggeredServiceRequestCMConnected
//
//#Description#
//    Verify that SMF sends ETSI PDU Session Modification Request for
//    Network Triggered Service Request when CM Connected.
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlow, TC79364_NetworkTriggeredServiceRequestCMConnected)
{
    banner("AN Release");
    Procedures::anRelease(optionalData, {session_, dedicatedQosBearers});

    banner("Network Triggered Service Request with CM-Connected");
    Procedures::networkTriggeredServiceRequestConnectionManagementConnected(optionalData,
                                                                            {session_, dedicatedQosBearers});
    std::vector<Bearer*> associatedQosFlowList = {};
    associatedQosFlowList.push_back(&session_.getDefaultBearer());
    std::copy(dedicatedQosBearers.begin(), dedicatedQosBearers.end(), back_inserter(associatedQosFlowList));
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosInActiveList(session_, associatedQosFlowList));
}


//*=============================================================================
// test case: TC79875_5gsToEpsDefaultEbiNotTransferred
//
//#Description#
//    Verify that SMF shall return 403 cause when the ebi value of QoS Flow
//    is associated with default QoS Rule and triggers PDU Session Release
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlow, TC79875_5gsToEpsDefaultEbiNotTransferred)
{
    banner("Mobility 5g To 4g Handover Failed");
    auto ebi = TestSupport::Types::EpsBearerId(session_.getDefaultBearer().bearerId_);
    Procedures::mobility5g4g.runActionsBefore("UpfReceiveSessionModificationRequest_CreatePdr",
                                              {std::vector<TestSupport::Types::EpsBearerId>{ebi}});
    amf_.receive(session_, amf_.createSmContextRetrieveFailureResponse(session_, TestSupport::StatusCode::Forbidden));
}


class LawfulInterceptionEtsiMultipleQosFlowWith5gsToEps : public LawfulInterceptionEtsiMultipleQosFlow
{
    void preTestCase() override
    {
        Sft::LiConfiguration liConfig{CurrentConfiguration::getInstance().getConfigurationBuilder()};
        piafGsc_.sendSetcInd(liConfig);
    }

protected:
    Sft::X2::Message& buildX2Message()
    {
        auto& x2Message     = etsiDf2_.pdnSessionModificationInterceptEvent(session_, std::nullopt);
        auto& modifyMessage = x2Message.get<LiEtsiTest::X2::PduSessionModificationIEs>();
        modifyMessage.epsPdnConnectionModification->indicationFlags = gtpv2::ie::IndicationFlags::OperationIndication;
        return x2Message;
    }

    void postTestCase() override {}
};
//*=============================================================================
// test case: TC79381_5gsToEpsHoWithN26
//
//#Description#
//    During 5G to 4G mobility the default QoS flow is transferred.
//    The dedicated QoS flow included in notToTransferEbiList is not
//    transferred and is deleted after the procedure finishes.
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlowWith5gsToEps, TC79381_5gsToEpsHoWithN26)
{
    session_.isEpsIwki_ = TestSupport::Types::EpsInterworkingInd::WithN26;
    const auto task     = Li::SessionTasks::Task6;

    // ========================
    // === Task added to DB ===
    // ========================
    addTaskToRedis(task);

    SftCore::Logger::banner("PDU Session establishment");
    create5gSession(task);

    verifyLiT3(task);
    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));


    // Create the non-GBR dedicated flow
    banner("createQosFlows");
    constexpr auto chargingRules = 5;
    const auto [definitions, bearerQoses] =
        Sft::Util::createDefinitionsAndQoses(Sft::Util::createNonGbrChargingRules(chargingRules));
    const ProcedureData optionalData{definitions, bearerQoses, task};

    const auto& dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);
    Procedures::createQosFlows(optionalData, {session_, dedicatedQosBearers});
    const auto qosRules = Sft::Util::createQosRuleDefinitions(
        optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::CreateNewQosRule);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(session_, dedicatedQosBearers, qosRules,
                                                                               Sft::Li::QosFlowInfo::FullInfo));
    lit3_.dontReceive();

    banner("5GS to EPS handover");
    constexpr auto notToTransferRules = 2;
    auto           bearerIds          = std::vector<TestSupport::Types::EpsBearerId>();
    for (auto i = 0; i < notToTransferRules; i++)
    {
        bearerIds.emplace_back(dedicatedQosBearers[i]->bearerId_);
    }
    const ProcedureData optionalDataNotToTransferEbiList{
        definitions, bearerQoses, task, bearerIds, EventTrigger::RatChange, EventTrigger::ServingCnNodeLocationChange};

    Procedures::mobility5g4g.runActionsBefore("UpfReceiveSessionModificationRequest_CreatePdr",
                                              optionalDataNotToTransferEbiList, {session_, dedicatedQosBearers});

    auto message = upf_.sessionModificationRequest(session_).addCreatePdr(
        Sft::pfcp::PdrHelper::uplink("PdrUnconditionalHandoverUplink")
            .setSourceInterface(Pfcp::SourceInterface::Access)
            .setNetworkInstance(UpfConstants::S5S8NetworkInstance)
            .setFarId(SftUpf::FarDefaultBearerUplink)
            .addQerId(SftUpf::QerApnAmbr)
            .setPrecedence(4294967295)
            .setApplicationId("default"));

    std::for_each(definitions.begin() + notToTransferRules, definitions.end(), [&](const auto definition) {
        message.addCreatePdr(pfcp::PdrHelper::uplinkForDcr(
            definition.getName(), definition,
            session_.getBearer(definition.getQosInformation()->getQosClassIdentifier().value(),
                               definition.getQosInformation()->getArp()->getPriority().value())));
    });

    upf_.receive(session_, message);

    upf_.send(session_, upf_.sessionModificationResponse(session_));

    Procedures::mobility5g4g.runActionsBetween("UpfReceiveSessionModificationRequest_CreatePdr",
                                               "PcfReceivePolicyUpdateRequest5gTo4gIwk",
                                               optionalDataNotToTransferEbiList, {session_, dedicatedQosBearers});

    Procedures::mobility5g4g.runActionsAfter("PcfSendPolicyUpdateResponse", optionalDataNotToTransferEbiList,
                                             {session_, dedicatedQosBearers});

    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptHandoverEvent(session_));

    banner("Delete QoS not Included in EBI List");
    const auto definitionsToDelete = std::vector(definitions.begin(), definitions.begin() + notToTransferRules);
    const auto bearerQosesToDelete = std::vector(bearerQoses.begin(), bearerQoses.begin() + notToTransferRules);
    const auto deleteDedicatedQosBearers =
        std::vector(dedicatedQosBearers.begin(), dedicatedQosBearers.begin() + notToTransferRules);
    const ProcedureData optionalDataBearersToDelete{definitionsToDelete, bearerQosesToDelete};
    Procedures::deleteBearersLocal(optionalDataBearersToDelete, {session_, deleteDedicatedQosBearers});
    verifyLiT3(task);
    // TODO: Verify IRI event does not contain excluded QoS Flow
    for (auto i = 0; i < notToTransferRules; i++)
    {
        etsiDf2_.receive(etsiDf2_.pdnSessionModificationInterceptEvent(session_, std::nullopt));
    }
    banner("Delete PDN Session");
    Procedures::deletePdnSession(optionalData, {session_, dedicatedQosBearers}, {-amf_});
    for (auto i = 0; i < (chargingRules - notToTransferRules); i++)
    {
        etsiDf2_.receive(buildX2Message());
    }
    etsiDf2_.receive(etsiDf2_.pdnSessionReleaseInterceptEvent(session_));
}
//*=============================================================================
// test case: TC79784_5GStoEps_IdleMobility_defaultQosToTransferEbiList
//
//#Description#
//    To verify 5G to 4G idle mobility success with multiple QoS flow
//    Default and dedicated QoS flows are created. During 5G to 4G idle mobility
//    The dedicated QoS flow is not
//    transferred and is deleted after the procedure finishes.
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlowWith5gsToEps, TC79784_5GStoEps_IdleMobility_defaultQosToTransferEbiList)
{
    session_.isEpsIwki_ = TestSupport::Types::EpsInterworkingInd::WithN26;
    const auto task     = Li::SessionTasks::Task6;

    // ========================
    // === Task added to DB ===
    // ========================
    addTaskToRedis(task);

    SftCore::Logger::banner("PDU Session establishment");
    create5gSession(task);

    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));
    verifyLiT3(task);

    // Create the non-GBR dedicated flow
    banner("createQosFlows");
    constexpr auto numberOfChargingRules = 5;
    const auto [definitions, bearerQoses] =
        Sft::Util::createDefinitionsAndQoses(Sft::Util::createNonGbrChargingRules(numberOfChargingRules));
    const ProcedureData optionalData{definitions, bearerQoses, task};

    const auto& dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);
    Procedures::createQosFlows(optionalData, {session_, dedicatedQosBearers});
    const auto qosRules = Sft::Util::createQosRuleDefinitions(
        optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::CreateNewQosRule);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(session_, dedicatedQosBearers, qosRules,
                                                                               Sft::Li::QosFlowInfo::FullInfo));
    lit3_.dontReceive();

    banner("anRelease");
    Procedures::anRelease(optionalData, {session_, dedicatedQosBearers});

    banner("5GS to EPS IdleMobility");
    auto           bearerIdsToDelete = std::vector<TestSupport::Types::EpsBearerId>();
    constexpr auto nbrToDelete       = numberOfChargingRules - 2;
    for (auto i = 0; i < nbrToDelete; i++)
    {
        bearerIdsToDelete.emplace_back(dedicatedQosBearers[i]->bearerId_);
    }
    const ProcedureData optionalDataNotToTransferEbiList{definitions,
                                                         bearerQoses,
                                                         task,
                                                         bearerIdsToDelete,
                                                         EventTrigger::RatChange,
                                                         EventTrigger::ServingCnNodeLocationChange};

    Procedures::mobility5g4g.runActionsBefore("UpfReceiveSessionModificationRequest_CreatePdr",
                                              optionalDataNotToTransferEbiList, {session_, dedicatedQosBearers});

    auto message = upf_.sessionModificationRequest(session_).addCreatePdr(
        Sft::pfcp::PdrHelper::uplink("PdrUnconditionalHandoverUplink")
            .setSourceInterface(Pfcp::SourceInterface::Access)
            .setNetworkInstance(UpfConstants::S5S8NetworkInstance)
            .setFarId(SftUpf::FarDefaultBearerUplink)
            .addQerId(SftUpf::QerApnAmbr)
            .setPrecedence(4294967295)
            .setApplicationId("default"));

    std::for_each(definitions.begin() + nbrToDelete, definitions.end(), [&](const auto definition) {
        message.addCreatePdr(pfcp::PdrHelper::uplinkForDcr(
            definition.getName(), definition,
            session_.getBearer(definition.getQosInformation()->getQosClassIdentifier().value(),
                               definition.getQosInformation()->getArp()->getPriority().value())));
    });

    upf_.receive(session_, message);

    upf_.send(session_, upf_.sessionModificationResponse(session_));

    Procedures::mobility5g4g.runActionsBetween("UpfReceiveSessionModificationRequest_CreatePdr",
                                               "PcfReceivePolicyUpdateRequest5gTo4gIwk",
                                               optionalDataNotToTransferEbiList, {session_, dedicatedQosBearers});

    Procedures::mobility5g4g.runActionsAfter("PcfSendPolicyUpdateResponse", optionalDataNotToTransferEbiList,
                                             {session_, dedicatedQosBearers});

    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptHandoverEvent(session_));


    banner("Delete QoS not Included in EBI List");
    const auto definitionsToDelete = std::vector(definitions.begin(), definitions.begin() + nbrToDelete);
    const auto bearerQosesToDelete = std::vector(bearerQoses.begin(), bearerQoses.begin() + nbrToDelete);
    const auto dedicatedQosBearersToDelete =
        std::vector(dedicatedQosBearers.begin(), dedicatedQosBearers.begin() + nbrToDelete);
    const ProcedureData optionalDataBearersToDelete{definitionsToDelete, bearerQosesToDelete};
    Procedures::deleteBearersLocal(optionalDataBearersToDelete, {session_, dedicatedQosBearersToDelete});

    verifyLiT3(task);
    // TODO: Verify IRI event does not contain excluded QoS Flow
    for (auto i = 0; i < nbrToDelete; i++)
    {
        etsiDf2_.receive(etsiDf2_.pdnSessionModificationInterceptEvent(session_, std::nullopt));
    }

    banner("Delete PDN Session");
    Procedures::deletePdnSession(optionalData, {session_, dedicatedQosBearers}, {-amf_});
    for (auto i = 0; i < numberOfChargingRules - nbrToDelete; i++)
    {
        etsiDf2_.receive(buildX2Message());
    }
    etsiDf2_.receive(etsiDf2_.pdnSessionReleaseInterceptEvent(session_));
}

class LawfulInterceptionEtsiMultipleQosFlowUpDeactivation : public LawfulInterceptionEtsiMultipleQosFlow
{
    void preTestCase() override {}
    void postTestCase() override {}

protected:
    static constexpr auto IdleTimeoutSec       = 30s;
    static constexpr auto InactivityTimeoutSec = 10s;
};

//*=============================================================================
// test case: TC80240_SmfTriggeredWithUpConnectionDeactivation
//
//#Description#
//    Test UP deactivation on inactivity timer expiration when
//    Session has dedicated QoS flow
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlowUpDeactivation, TC80240_SmfTriggeredWithUpConnectionDeactivation)
{
    auto& configuration = CurrentConfiguration::getInstance().getConfigurationBuilder();

    configuration.getPgw()
        .getApn("apn1.com")
        .getApnPdpContext()
        .getApnSessionControl()
        .setPduSessionInactivityTimer(InactivityTimeoutSec);

    configuration.getPgw()
        .getApn("apn1.com")
        .getApnPdpContext()
        .getApnSessionControl()
        .getIdleTimeout()
        .getDefault()
        .setTimeout(IdleTimeoutSec);

    ConfigurationUtil::commit(lcmd_, configuration);

    const auto task = Li::SessionTasks::Task6;
    // ========================
    // === Task added to DB ===
    // ========================
    addTaskToRedis(task);

    SftCore::Logger::banner("Pdu Session establishment");
    Procedures::establish5gSession.runActionsBefore("redisReceiveTargetInfoRequest",
                                                    {::Sft::pfcp::UserPlaneInactivityTimer{InactivityTimeoutSec}});
    liRedis_.receive(session_, liRedis_.targetInfoRequest(session_));
    liRedis_.send(liRedis_.targetInfoEtsiReply(session_, {task}));
    Procedures::establish5gSession.runActionsBetween("redisSendTargetInfoResponse",
                                                     "Df2ReceivePduSessionEstablishmentRequest",
                                                     {::Sft::pfcp::UserPlaneInactivityTimer{InactivityTimeoutSec}});

    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    verifyLiT3(task);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));

    // Create the non-GBR dedicated flow
    const auto [definitions, bearerQoses] =
        Sft::Util::createDefinitionsAndQoses(Sft::Util::createNonGbrChargingRules(9));
    const ProcedureData optionalData{definitions, bearerQoses, task};
    const auto&         dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);

    banner("createQosFlows");
    Procedures::createQosFlows(optionalData, {session_, dedicatedQosBearers});
    const auto qosRules = Sft::Util::createQosRuleDefinitions(
        optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::CreateNewQosRule);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(session_, dedicatedQosBearers, qosRules,
                                                                               Sft::Li::QosFlowInfo::FullInfo));
    lit3_.dontReceive();

    banner("inactivity-Timeout expired");
    Procedures::anReleaseDuetoInactivityTimeout.runActionsBefore(
        "AmfSendN1n2MessageTransferResponse", {::Sft::pfcp::UserPlaneInactivityTimer{IdleTimeoutSec}},
        {session_, dedicatedQosBearers});
    amf_.send(session_,
              amf_.n1n2MessageTransferResponse(session_, TestSupport::StatusCode::Ok,
                                               TestSupport::Types::N1N2MessageTransferCause::N1N2TransferInitiated));

    Time::run(2000ms + Time::tick);

    Procedures::anReleaseDuetoInactivityTimeout.runActionsAfter("AmfSendN1n2MessageTransferResponse",
                                                                {::Sft::pfcp::UserPlaneInactivityTimer{IdleTimeoutSec}},
                                                                {session_, dedicatedQosBearers});
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEvent(session_));

    banner("upf send sessionReport due to idle-Timeout expired");
    upf_.send(session_, upf_.sessionReportRequest(session_, Pfcp::ReportType::Type::UserPlaneInactivityReport));
    upf_.receive(session_, upf_.sessionReportResponse(session_));

    Procedures::release5gSessionNetworkTriggeredWithUpDeactivated();
    etsiDf2_.receive(etsiDf2_.pduSessionReleaseInterceptEvent(session_));
}


//*=============================================================================
// test case: TC80498_5gsToEps_IdleMobility_DefaultEbiNotTransferred
//
//#Description#
//    During 5GS to EPS Idle Mobility,Verify that SMF shall return 403 cause
//    when the ebi value of QoS Flow is associated with default QoS Rule
//    and triggers PDU Session Release
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlowWith5gsToEps, TC80498_5gsToEps_IdleMobility_DefaultEbiNotTransferred)
{
    // ========================
    // === Task added to DB ===
    // ========================
    addTaskToRedis(Li::SessionTasks::Task6);

    SftCore::Logger::banner("Pdu Session establishment");
    create5gSession(Li::SessionTasks::Task6);
    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    verifyLiT3(Li::SessionTasks::Task6);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));

    // Create the non-GBR dedicated flow
    constexpr auto numFlows = 9;
    const auto [definitions, bearerQoses] =
        Sft::Util::createDefinitionsAndQoses(Sft::Util::createNonGbrChargingRules(numFlows));
    optionalData        = ProcedureData{definitions, bearerQoses, Li::SessionTasks::Task6};
    dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);

    banner("createQosFlows");
    Procedures::createQosFlows(optionalData, {session_, dedicatedQosBearers});
    const auto qosRules = Sft::Util::createQosRuleDefinitions(
        optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::CreateNewQosRule);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(session_, dedicatedQosBearers, qosRules,
                                                                               Sft::Li::QosFlowInfo::FullInfo));
    lit3_.dontReceive();

    banner("anRelease");
    Procedures::anRelease(optionalData, {session_, dedicatedQosBearers});

    banner("Mobility 5g To 4g Handover Failed");
    auto ebi = TestSupport::Types::EpsBearerId(session_.getDefaultBearer().bearerId_);
    Procedures::mobility5g4g.runActionsBefore("UpfReceiveSessionModificationRequest_CreatePdr",
                                              {std::vector<TestSupport::Types::EpsBearerId>{ebi}});
    amf_.receive(session_, amf_.createSmContextRetrieveFailureResponse(session_, TestSupport::StatusCode::Forbidden));

    lit3_.dontReceive();
    // =======================
    // === Session Release ===
    // =======================
    SftCore::Logger::banner("Pdu Session Release");
    Procedures::release5gSessionWithUpDeactivated();
    etsiDf2_.receive(etsiDf2_.pduSessionReleaseInterceptEvent(session_));
    lit3_.dontReceive();
}

class LawfulInterceptionEtsiMultipleQosFlowXnHandover : public LawfulInterceptionEtsiMultipleQosFlow
{
    void preTestCase() override {}
    void postTestCase() override {}
};
//*=============================================================================
// test case: TC80331_XnHandoverWithoutIUpf
//
// Verify MultipleQosFLow with Xn Handover
//
//#Description#
//    To verify Multiple QoS Flow with Xn Handover
//    1. Establish 5G session
//    2. Create Non-GBR dedicated flow
//    3. Perform Xn-Handover and Verify PduSessionModification
//    4. Release 5G Session
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlowXnHandover, TC80331_XnHandoverWithoutIUpf)
{
    session_.isEpsIwki_ = TestSupport::Types::EpsInterworkingInd::WithN26;
    const auto task     = Li::SessionTasks::Task6;
    // =================================
    // === PDU Session Establishment ===
    // =================================
    addTaskToRedis(task);
    create5gSessionWithPcf(task);
    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    verifyLiT3(task);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));

    // Create the non-GBR dedicated flow

    banner("createQosFlows");
    constexpr auto chargingRules = 5;
    const auto [definitions, bearerQoses] =
        Sft::Util::createDefinitionsAndQoses(Sft::Util::createNonGbrChargingRules(chargingRules));
    const ProcedureData optionalData{definitions, bearerQoses, task};

    const auto& dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);
    const auto  qosRules            = Sft::Util::createQosRuleDefinitions(
                    optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::CreateNewQosRule);
    Procedures::createQosFlows(optionalData, {session_, dedicatedQosBearers});
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(session_, dedicatedQosBearers, qosRules,
                                                                               Sft::Li::QosFlowInfo::FullInfo));
    // ===================
    // === Xn Handover ===
    // ===================

    banner("Start Xn Handover");
    session_.tac_.value() += 1;
    auto& httpMessage = amf_.updateSmContextRequest_PathSwitch(session_);
    httpMessage.getJsonBuilder().setServingNetwork(TestSupport::Types::PlmnId(session_.ue_.mccMnc_));

    amf_.send(session_, httpMessage);
    const auto transportInfo = httpMessage.getN2SmInfoAs<N2::PathSwitchRequestTransfer>()
                                   .getDlNguUpTransportLayerInformation()
                                   .transportLayerInformation_;
    const auto ipAddress = transportInfo.ipAddress_;
    const auto teid      = transportInfo.teid_;


    pcf_.receive(session_,
                 N7::MessageBuilder{pcf_.policyUpdateRequest(session_)}
                     .setUserLocationInfo(TestSupport::Types::UserLocation(TestSupport::Types::NrLocation(
                         session_.ue_.mccMnc_, session_.nci_.value(), session_.tac_.value())))
                     .addPolicyControlRequestTrigger(N7::PolicyControlRequestTrigger::ServingAreaLocationChange));

    pcf_.send(session_, pcf_.policyUpdateResponse(session_, TestSupport::StatusCode::NoContent));

    banner("SMF Query URRs and update downlink FARs");
    const auto downlinkForwardParameters = pfcp::ForwardingParameters()
                                               .setDestinationInterface(Pfcp::DestinationInterface::Access)
                                               .setPfcpSmReqFlag(::Pfcp::PfcpSmReqFlagsType::SendEndMarkerPackets)
                                               .setNetworkInstance(UpfConstants::N3NetworkInstance)
                                               .setOuterHeaderCreationGtpWithPort(teid, ipAddress);
    auto updateFar = [&](const auto& farName) {
        return Far(farName)
            .addApplyAction(Pfcp::ApplyAction::Action::Forward)
            .setForwardingParameters(downlinkForwardParameters);
    };

    auto modificationRequest = upf_.sessionModificationRequest(session_);
    modificationRequest.addUpdateFar(updateFar(FarDefaultBearerDownlink));
    for (const auto& qosBearer : dedicatedQosBearers)
    {
        const std::string farName = pfcp::FarHelper::getFarNamesForBearer(*qosBearer)[1].getName();
        modificationRequest.addUpdateFar(updateFar(farName));
    }
    upf_.receive(session_, modificationRequest);

    upf_.send(session_, upf_.sessionModificationResponse(session_));
    amf_.receive(session_, amf_.updateSmContextResponse_PathSwitchRequestAcknowledgeTransfer(
                               session_, TestSupport::StatusCode::Ok));

    std::vector<Bearer*> bearers;
    for (const auto& uniquePtr : session_.getAllBearers())
    {
        bearers.push_back(uniquePtr.get());
    }
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(session_, bearers, qosRules,
                                                                               Sft::Li::QosFlowInfo::OnlyQfi));

    // =======================
    // === Session Release ===
    // =======================
    Procedures::release5gSession();
    etsiDf2_.receive(etsiDf2_.pduSessionReleaseInterceptEvent(session_));
}

//*=============================================================================
// test case: TC80346_5gsToEpsHoWithN26NewPccRules
//
// To verify 5G to 4G Handover with multiple Qos flows where PCF provisions new PCC rules/session rules
// when the RAT Type is changed.
//
//#Description#
//    - Establish PDU session with dedicated Qos flows established in 5GS.
//    - AMF triggers 5GS to Eps Handover and sends Context Retrive Request to SMF
//    - PCF is provisions to install new dynamic rule before handover is completed
//    - Dedicated bearers are installed due to PCF provisioned new rule to be installed during HO
//    - Release the PDN session in 4G
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlowWith5gsToEps, TC80346_5gsToEpsHoWithN26NewPccRules)
{
    session_.isEpsIwki_                        = TestSupport::Types::EpsInterworkingInd::WithN26;
    constexpr auto        inactivityTimeoutSec = 10s;
    static constexpr auto idleTimeoutSec       = 30s;
    const auto            task                 = Li::SessionTasks::Task6;
    auto&                 configuration        = CurrentConfiguration::getInstance().getConfigurationBuilder();

    configuration.getPgw()
        .getApn("apn1.com")
        .getApnPdpContext()
        .getApnSessionControl()
        .setPduSessionInactivityTimer(inactivityTimeoutSec);

    configuration.getPgw()
        .getApn("apn1.com")
        .getApnPdpContext()
        .getApnSessionControl()
        .getIdleTimeout()
        .getDefault()
        .setTimeout(idleTimeoutSec);

    ConfigurationUtil::commit(lcmd_, configuration);

    // ========================
    // === Task added to DB ===
    // ========================
    addTaskToRedis(task);

    SftCore::Logger::banner("PDU Session establishment");

    Procedures::establish5gSession.runActionsBefore(
        "redisReceiveTargetInfoRequest",
        {::Sft::pfcp::UserPlaneInactivityTimer{inactivityTimeoutSec}, EventTrigger::RatChange});
    liRedis_.receive(session_, liRedis_.targetInfoRequest(session_));
    liRedis_.send(liRedis_.targetInfoEtsiReply(session_, {task}));
    Procedures::establish5gSession.runActionsBetween(
        "redisSendTargetInfoResponse", "Df2ReceivePduSessionEstablishmentRequest",
        {::Sft::pfcp::UserPlaneInactivityTimer{inactivityTimeoutSec}, EventTrigger::RatChange});

    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    verifyLiT3(task);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));
    // Create the non-GBR dedicated flow
    banner("createQosFlows");
    constexpr auto chargingRules = 2;
    const auto [definitions, bearerQoses] =
        Sft::Util::createDefinitionsAndQoses(Sft::Util::createNonGbrChargingRules(chargingRules));
    const ProcedureData optionalData{definitions, bearerQoses, task};
    const auto          qosRules = Sft::Util::createQosRuleDefinitions(
                 optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::CreateNewQosRule);
    const auto& dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);
    Procedures::createQosFlows(optionalData, {session_, dedicatedQosBearers});
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(session_, dedicatedQosBearers, qosRules,
                                                                               Sft::Li::QosFlowInfo::FullInfo));
    lit3_.dontReceive();

    banner("5gsToEps Handover Start");
    amf_.send(session_, amf_.createSmContextRetrieveRequest(session_));

    upf_.receive(session_,
                 upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));

    upf_.send(session_, upf_.sessionModificationResponse(session_));

    verifyLiT3(task);

    amf_.receive(session_, amf_.createSmContextRetrieveResponse(session_, TestSupport::StatusCode::Ok));

    for (auto& bearer : session_.getAllBearers())
    {
        bearer->servingNode_.uFTeid_.setTeid(Sft::Generator::getInstance().newTeid());
    }

    session_.servingNode_.cFTeid_ = TestSupport::Types::CFTeid(
        TestSupport::Types::IpAddress(sgw_.getServingNodeCAddressV4(), sgw_.getServingNodeCAddressV6()),
        Sft::Generator::getInstance().newTeid());

    gtpv2::message::Message& message =
        sgw_.modifyBearerRequest(session_).set<gtpv2::ie::RatType>(GtpCommon::RatType::Eutran);
    sgw_.send(sgw_.modifyBearerRequestWithUteidAllBearer(session_, message));


    pcf_.receive(session_, N7::MessageBuilder{pcf_.policyUpdateRequest(session_)}
                               .addPolicyControlRequestTrigger(N7::PolicyControlRequestTrigger::RatTypeChange)
                               .setRatType(N7::RatType::Eutra));
    pcf_.send(session_, pcf_.policyUpdateResponse(session_, TestSupport::StatusCode::NoContent));

    // Dcr with GBR
    auto dcr = chargingRules_.rules_[4].definition_;

    std::for_each(definitions.begin(), definitions.end(),
                  [dcr](const auto definition) { EXPECT_NE(dcr.getName(), definition.getName()); });

    // Install new DCR during procedure. Should be triggered after HO...
    auto notifyRequestNewRule =
        N7::chargingRulesToPccRules(N7::MessageBuilder{pcf_.policyNotifyRequest(session_)}, {&dcr});
    pcf_.send(session_, notifyRequestNewRule);

    auto pfcpmessage =
        UpfMessageFactory::sessionModificationRequestDownlinkFor5gHandover4g(upf_, sgw_, session_, dedicatedQosBearers)
            .setUserPlaneInactivityTimer(idleTimeoutSec)
            .setVerificationLevel(VerificationLevel::OnlySpecified);

    upf_.receive(session_, pfcpmessage);
    upf_.send(session_, upf_.sessionModificationResponse(session_));
    verifyLiT3(task);

    for (auto& bearer : session_.getAllBearers())
    {
        bearer->servingNode_.uFTeid_.setTeid(Sft::Generator::getInstance().newTeid());
    }
    sgw_.receive(session_,
                 sgw_.modifyBearerResponseWithUteid(session_)
                     .add<gtpv2::ie::BearerContext>()
                     .addToGroup<gtpv2::ie::BearerContext, gtpv2::ie::Ebi>(session_.getAllBearers()[1]->bearerId_)
                     .addToGroup<gtpv2::ie::BearerContext, gtpv2::ie::Cause>(gtpv2::CauseCode::RequestAccepted)
                     .addToGroup<gtpv2::ie::BearerContext, gtpv2::ie::ChargingId>(
                         session_.getAllBearers()[1]->servingNode_.chargingId_)
                     .add<gtpv2::ie::BearerContext>()
                     .addToGroup<gtpv2::ie::BearerContext, gtpv2::ie::Ebi>(session_.getAllBearers()[2]->bearerId_)
                     .addToGroup<gtpv2::ie::BearerContext, gtpv2::ie::Cause>(gtpv2::CauseCode::RequestAccepted)
                     .addToGroup<gtpv2::ie::BearerContext, gtpv2::ie::ChargingId>(
                         session_.getAllBearers()[2]->servingNode_.chargingId_));

    pcf_.receive(session_, pcf_.policyNotifyResponse(session_, TestSupport::StatusCode::NoContent));

    upf_.receive(session_,
                 upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));

    upf_.send(session_, upf_.sessionModificationResponse(session_));
    banner("Handover completed");
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptHandoverEvent(session_));

    banner("Install dedicated bearer");
    // The procedure to install new rules is triggered after HO is done.
    Sft::Bearer& bearer   = installDedicatedBearerinEps();
    auto& expectedMessage = etsiDf2_.pdnSessionModificationInterceptEvent(session_, std::vector<Sft::Bearer*>{&bearer});
    auto& modifiedMessage = expectedMessage.get<LiEtsiTest::X2::PduSessionModificationIEs>();
    modifiedMessage.epsPdnConnectionModification->ePSBearerContextCreated->at(0).gTPTunnelInfo_ =
        LiEtsiTest::X2::GtpTunnelInfo{};  // Should contain epsGtpTunnels but not implemented yet
    etsiDf2_.receive(expectedMessage);
    verifyLiT3(task);

    banner("PDN Session Release");
    Procedures::deletePdnSession.runActionsBefore("PcfReceivePolicyDeleteRequest", {optionalData},
                                                  {session_, dedicatedQosBearers}, {-amf_});
    // Note, ratType is NR, is this correct?
    pcf_.receive(session_, pcf_.policyDeleteRequest(session_));
    Procedures::deletePdnSession.runActionsAfter("PcfReceivePolicyDeleteRequest");
    for (auto i = 0; i < chargingRules; i++)
    {
        etsiDf2_.receive(buildX2Message());
    }
    etsiDf2_.receive(buildX2Message());
    etsiDf2_.receive(etsiDf2_.pdnSessionReleaseInterceptEvent(session_));
}

//*=============================================================================
// test case: TC80978_N2HandoverAmfNoChange
//
//#Description#
//     Verify that SMF sends an ETSI PDU Session Modification Request
//     for a N2 Handover without AmfChange with all successful QoS flows.
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlow, TC80978_N2HandoverAmfNoChange)
{
    banner("Start N2 HO DirectDataForwarding");
    Procedures::n2HandoverDirectDataForwardingWithMultipleQos(optionalData, {session_, dedicatedQosBearers});

    auto& defaultBearer = session_.getBearerByBearerId(session_.getDefaultBearer().bearerId_);

    std::vector<Bearer*> defaultDedicatedBearers({&defaultBearer});
    defaultDedicatedBearers.insert(defaultDedicatedBearers.end(), dedicatedQosBearers.begin(),
                                   dedicatedQosBearers.end());
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventN2HandoverWithQosFlows(
        session_, defaultDedicatedBearers, LiEtsiTest::X2::HandoverState::Completed));



       banner("install 2 pccrule on one qos flow");
    std::vector<BasicChargingRules::ChargingRule> rules({
        {// rules_[0]
         "DCR1",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         1,  // RatingGroup
         1,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Downlink),
          Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Uplink)}},
        {// rules_[1]
         "DCR2",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         65,  // RatingGroup
         2,   // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.8 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
        {// rules_[2]
         "DCR3",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         8,  // RatingGroup
         3,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.65 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
    });

    auto& dcr1 = rules[0].definition_;
    auto& dcr2 = rules[1].definition_;
    auto& dcr3 = rules[2].definition_;
    // DCR1&DCR2   ----> QCI 3
    auto                bearerQoses = Qos::generateBearerQos({
                       dcr1,
                       dcr2,
    });

    banner("remove DCR1, update DCR2, install DCR3");

    
    const auto& qos             = dcr1.getQosInformation();
    auto&       dedicatedBearer = session_.getBearer(*qos->getQosClassIdentifier(), *qos->getArp()->getPriority());


    std::vector<const Gx::ChargingRuleDefinition*> chargingRuleDefinitions;
    chargingRuleDefinitions.emplace_back(&dcr3);

    pcf_.send(session_, N7::chargingRulesToPccRules(
                            N7::MessageBuilder{pcf_.policyNotifyRequest(session_)},
                            chargingRuleDefinitions));
    pcf_.receive(session_, pcf_.policyNotifyResponse(session_, TestSupport::StatusCode::NoContent));

    pfcp::MessageBuilder sessionModificationRequest = pfcp::MessageBuilder(upf_.sessionModificationRequest(session_));
    sessionModificationRequest.addCreatePdr(pfcp::PdrHelper::uplink(dcr3, session_.getDefaultBearer())
                          .setPrecedence(*dcr3.getPrecedence())
                          .setNetworkInstance(UpfConstants::N3NetworkInstance)
                          .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
                          .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
                          .addQerId(QerHelper::getQerNameForDcr(dcr3)))
        .addCreateQer(Qer{QerHelper::getQerNameForDcr(dcr3)}
                          .setMbr(TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateDownlink_))
                          .setGbr(TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateDownlink_)));

    upf_.receive(session_, sessionModificationRequest);
    upf_.send(session_, upf_.sessionModificationResponse(session_));

    // build IEI N1 QoSRules to send
    ::TestSupport::Types::QosRules qosRules;


    // create a rule
    qosRules.addQosRule(
        ::TestSupport::Types::QosRule()
            .setQosRuleId(::Compare::MatchAnyValue)  // Creating DCR 3, QRI unknown until created
            .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::CreateNewQosRule)
            .setPrecedence(*dcr3.getPrecedence())
            .setQfi(dedicatedBearer.qfi_)
            .setDefaultQosRule(false)
            .setSegregation(0)
            .addPacketFilter(
                ::TestSupport::Types::PacketFilter()
                    .setIdentifier(::Compare::MatchAnyValue)
                    .setDirection(::TestSupport::Types::PacketFilterDirection::Bidirectional)
                    .addComponent(::TestSupport::Types::PacketFilterComponent::Ipv4RemoteAddress(
                        ::TestSupport::Types::IpAddress("1.1.1.65"),
                        ::TestSupport::Types::IpAddress("255.255.255.255")))
                    .addComponent(::TestSupport::Types::PacketFilterComponent::ProtocolIdentifierNextHeader(
                        ::TestSupport::Types::IpProtocol::Udp))));

    N11::Message& message = amf_.n1n2MessageTransferRequest_ResourceModifyRequest_Basic(session_, qosRules);

    amf_.receive(session_, message);
    amf_.send(session_,
              amf_.n1n2MessageTransferResponse(session_, TestSupport::StatusCode::Ok,
                                               TestSupport::Types::N1N2MessageTransferCause::N1N2TransferInitiated));

    amf_.send(session_, amf_.updateSmContextRequest_ResourceModifyResponseTransfer(session_));
    amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    amf_.send(session_, amf_.updateSmContextRequest_SessionModificationComplete(session_));
    amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    upf_.receive(
        session_,
        upf_.sessionModificationRequest(session_)
            .addCreatePdr(
                pfcp::PdrHelper::downlink(dcr3)
                    .setPrecedence(*dcr3.getPrecedence())
                    .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
                    .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
                    .addQerId(QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
                    .addQerId(QerHelper::getQerNameForDcr(dcr3))));

    upf_.send(session_, upf_.sessionModificationResponse(session_));
}



TEST_F(LawfulInterceptionEtsiMultipleQosFlow, TC80978_N2HandoverAmfNoChange1)
{
 
   
       banner("install 2 pccrule on one qos flow");
    std::vector<BasicChargingRules::ChargingRule> rules({
        {// rules_[0]
         "DCR4",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(5), 1100_kb, 2100_kb, 100_kb, 200_kb},
         1,  // RatingGroup
         10,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Downlink),
          Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Uplink)}},
        {// rules_[1]
         "DCR5",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(5), 1100_kb, 2100_kb, 100_kb, 200_kb},
         65,  // RatingGroup
         11,   // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.8 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
        {// rules_[2]
         "DCR6",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(5), 1100_kb, 2100_kb, 100_kb, 200_kb},
         8,  // RatingGroup
         12,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.65 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
    });

    auto& dcr1 = rules[0].definition_;
    auto& dcr2 = rules[1].definition_;
    auto& dcr3 = rules[2].definition_;
    // DCR1&DCR2   ----> QCI 3
    auto                bearerQoses = Qos::generateBearerQos({
                       dcr1,
                       dcr2,
    });

    banner("remove DCR1, update DCR2, install DCR3");

    
    const auto& qos             = dcr1.getQosInformation();
    auto&       dedicatedBearer = session_.getBearer(*qos->getQosClassIdentifier(), *qos->getArp()->getPriority());


    std::vector<const Gx::ChargingRuleDefinition*> chargingRuleDefinitions;
    chargingRuleDefinitions.emplace_back(&dcr3);

    pcf_.send(session_, N7::chargingRulesToPccRules(
                            N7::MessageBuilder{pcf_.policyNotifyRequest(session_)},
                            chargingRuleDefinitions));
    pcf_.receive(session_, pcf_.policyNotifyResponse(session_, TestSupport::StatusCode::NoContent));

    pfcp::MessageBuilder sessionModificationRequest = pfcp::MessageBuilder(upf_.sessionModificationRequest(session_));
    sessionModificationRequest.addCreatePdr(pfcp::PdrHelper::uplink(dcr3, session_.getDefaultBearer())
                          .setPrecedence(*dcr3.getPrecedence())
                          .setNetworkInstance(UpfConstants::N3NetworkInstance)
                          .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
                          .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
                          .addQerId(QerHelper::getQerNameForDcr(dcr3)))
        .addCreateQer(Qer{QerHelper::getQerNameForDcr(dcr3)}
                          .setMbr(TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateDownlink_))
                          .setGbr(TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateDownlink_)));

     upf_.receive(session_,
                     upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));
    upf_.send(session_, upf_.sessionModificationResponse(session_));

    verifyLiT3(Li::SessionTasks::Task6);

    // build IEI N1 QoSRules to send
    // ::TestSupport::Types::QosRules qosRules;


    // // create a rule
    // qosRules.addQosRule(
    //     ::TestSupport::Types::QosRule()
    //         .setQosRuleId(::Compare::MatchAnyValue)  // Creating DCR 3, QRI unknown until created
    //         .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::CreateNewQosRule)
    //         .setPrecedence(*dcr3.getPrecedence())
    //         .setQfi(dedicatedBearer.qfi_)
    //         .setDefaultQosRule(false)
    //         .setSegregation(0)
    //         .addPacketFilter(
    //             ::TestSupport::Types::PacketFilter()
    //                 .setIdentifier(::Compare::MatchAnyValue)
    //                 .setDirection(::TestSupport::Types::PacketFilterDirection::Bidirectional)
    //                 .addComponent(::TestSupport::Types::PacketFilterComponent::Ipv4RemoteAddress(
    //                     ::TestSupport::Types::IpAddress("1.1.1.65"),
    //                     ::TestSupport::Types::IpAddress("255.255.255.255")))
    //                 .addComponent(::TestSupport::Types::PacketFilterComponent::ProtocolIdentifierNextHeader(
    //                     ::TestSupport::Types::IpProtocol::Udp))));

    // N11::Message& message = amf_.n1n2MessageTransferRequest_ResourceModifyRequest_Basic(session_, qosRules);

    // amf_.receive(session_, message);
    // amf_.send(session_,
    //           amf_.n1n2MessageTransferResponse(session_, TestSupport::StatusCode::Ok,
    //                                            TestSupport::Types::N1N2MessageTransferCause::N1N2TransferInitiated));

    // amf_.send(session_, amf_.updateSmContextRequest_ResourceModifyResponseTransfer(session_));
    // amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    // amf_.send(session_, amf_.updateSmContextRequest_SessionModificationComplete(session_));
    // amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    // upf_.receive(
    //     session_,
    //     upf_.sessionModificationRequest(session_)
    //         .addCreatePdr(
    //             pfcp::PdrHelper::downlink(dcr3)
    //                 .setPrecedence(*dcr3.getPrecedence())
    //                 .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
    //                 .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr3))));

    // upf_.send(session_, upf_.sessionModificationResponse(session_));
    Procedures::release5gSession();
}




TEST_F(LawfulInterceptionEtsiMultipleQosFlowXnHandover, TC81134_N2HandoverWithAmfChange)
{


    const auto task     = Li::SessionTasks::Task6;
    // =================================
    // === PDU Session Establishment ===
    // =================================
    addTaskToRedis(task);
    create5gSessionWithPcf(task);
    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    verifyLiT3(task);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));


      banner("install 2 pccrule on one qos flow");
    std::vector<BasicChargingRules::ChargingRule> rules({
        {// rules_[0]
         "DCR1",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         1,  // RatingGroup
         1,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Downlink),
          Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Uplink)}},
        {// rules_[1]
         "DCR2",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         65,  // RatingGroup
         2,   // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.8 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
        {// rules_[2]
         "DCR3",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         8,  // RatingGroup
         3,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.65 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
    });

    auto& dcr1 = rules[0].definition_;
    auto& dcr2 = rules[1].definition_;
    auto& dcr3 = rules[2].definition_;
    // DCR1&DCR2   ----> QCI 3
    auto                bearerQoses = Qos::generateBearerQos({
                       dcr1,
                       dcr2,
    });
    const ProcedureData optionalData{dcr1, dcr2, bearerQoses,Li::SessionTasks::Task6};
     // createQosFlows(optionalData);
    std::vector<Bearer*> bearers = Sft::Util::createQosFlowBearers(optionalData, session_);
    //auto dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);
    Procedures::createQosFlows(optionalData, {session_, bearers});
    

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsBefore("AmfReceiveUpdateSmContextResponseHandoverCommandTransferDirectDataForwardingWithMultipleQos", optionalData, {session_, bearers});

    banner("remove DCR1, update DCR2, install DCR3");

    const auto& qos             = dcr1.getQosInformation();
    auto&       dedicatedBearer = session_.getBearer(*qos->getQosClassIdentifier(), *qos->getArp()->getPriority());

    const auto oldPrecedenceOfDcr2 = dcr2.getPrecedence().value();
    dcr2.setFlowStatus(Avpvalue::FlowStatus::Disabled);
    dcr2.setPrecedence(5);

    std::vector<const Gx::ChargingRuleDefinition*> chargingRuleDefinitions;
    chargingRuleDefinitions.emplace_back(&dcr2);
    chargingRuleDefinitions.emplace_back(&dcr3);

    pcf_.send(session_, N7::chargingRulesToPccRules(
                            N7::MessageBuilder{pcf_.policyNotifyRequest(session_)}.removePccRule(dcr1.getName()),
                            chargingRuleDefinitions));
    pcf_.receive(session_, pcf_.policyNotifyResponse(session_, TestSupport::StatusCode::NoContent));

    pfcp::MessageBuilder sessionModificationRequest = pfcp::MessageBuilder(upf_.sessionModificationRequest(session_));
    sessionModificationRequest.addRemovePdr(PdrHelper::getPdrDownlinkNameForDcr(dcr1))
        .addUpdatePdr(
            Pdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr2, dedicatedBearer))
                // todo : need to set precedence to 5(new precedence)
                //.setPrecedence(dcr.getPrecedence().value())
                .setPrecedence(2)
                .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
                .setNetworkInstance(UpfConstants::N3NetworkInstance)
                .addQerId(QerHelper::getQerNameForDcr(dcr2))
                .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned"))
                .setOuterHeaderRemoval(Pfcp::OuterHeaderRemoval(Pfcp::OuterHeaderRemovalType::GtpuUdpIp)))
        .addUpdatePdr(
            PdrHelper::downlink(dcr2)
                // todo : need to set precedence to 5(new precedence)
                //.setPrecedence(dcr.getPrecedence().value())
                .setPrecedence(2)
                .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
                .addQerId(QerHelper::getQerNameForDcr(dcr2))
                .addQerId(Sft::pfcp::QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
                .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned")))
        .addCreatePdr(pfcp::PdrHelper::uplink(dcr3, session_.getDefaultBearer())
                          .setPrecedence(*dcr3.getPrecedence())
                          .setNetworkInstance(UpfConstants::N3NetworkInstance)
                          .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
                          .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
                          .addQerId(QerHelper::getQerNameForDcr(dcr3)))
        .addCreateQer(Qer{QerHelper::getQerNameForDcr(dcr3)}
                          .setMbr(TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateDownlink_))
                          .setGbr(TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateDownlink_)));

    //upf_.receive(session_, sessionModificationRequest);


     upf_.receive(session_,
                     upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));
    upf_.send(session_, upf_.sessionModificationResponse(session_));

    // build IEI N1 QoSRules to send
    ::TestSupport::Types::QosRules qosRules;

    // remove a rule
    qosRules.addQosRule(
        ::TestSupport::Types::QosRule()
            .setQosRuleId(session_.amf_.getQosRuleIdForPccRule(*dcr1.getPrecedence()).value())
            .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::DeleteExistingQosRule)
            .setDefaultQosRule(false));

    // update a rule
    qosRules.addQosRule(
        ::TestSupport::Types::QosRule()
            .setQosRuleId(session_.amf_.getQosRuleIdForPccRule(oldPrecedenceOfDcr2).value())
            .setRuleOperationCode(
                ::TestSupport::Types::QosRule::RuleOperationCode::ModifyExistingQosRuleWithoutModifyingPacketFilters)
            .setPrecedence(*dcr2.getPrecedence())
            .setQfi(dedicatedBearer.qfi_)
            .setDefaultQosRule(false)
            .setSegregation(0));

    // create a rule
    qosRules.addQosRule(
        ::TestSupport::Types::QosRule()
            .setQosRuleId(::Compare::MatchAnyValue)  // Creating DCR 3, QRI unknown until created
            .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::CreateNewQosRule)
            .setPrecedence(*dcr3.getPrecedence())
            .setQfi(dedicatedBearer.qfi_)
            .setDefaultQosRule(false)
            .setSegregation(0)
            .addPacketFilter(
                ::TestSupport::Types::PacketFilter()
                    .setIdentifier(::Compare::MatchAnyValue)
                    .setDirection(::TestSupport::Types::PacketFilterDirection::Bidirectional)
                    .addComponent(::TestSupport::Types::PacketFilterComponent::Ipv4RemoteAddress(
                        ::TestSupport::Types::IpAddress("1.1.1.65"),
                        ::TestSupport::Types::IpAddress("255.255.255.255")))
                    .addComponent(::TestSupport::Types::PacketFilterComponent::ProtocolIdentifierNextHeader(
                        ::TestSupport::Types::IpProtocol::Udp))));

    N11::Message& message = amf_.n1n2MessageTransferRequest_ResourceModifyRequest_Basic(session_, qosRules);

    amf_.receive(session_, message);
    amf_.send(session_,
              amf_.n1n2MessageTransferResponse(session_, TestSupport::StatusCode::Ok,
                                               TestSupport::Types::N1N2MessageTransferCause::N1N2TransferInitiated));

    amf_.send(session_, amf_.updateSmContextRequest_ResourceModifyResponseTransfer(session_));
    amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    amf_.send(session_, amf_.updateSmContextRequest_SessionModificationComplete(session_));
    amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    // upf_.receive(
    //     session_,
    //     upf_.sessionModificationRequest(session_)
    //         .addCreatePdr(
    //             pfcp::PdrHelper::downlink(dcr3)
    //                 .setPrecedence(*dcr3.getPrecedence())
    //                 .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
    //                 .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr3)))
    //         .addUpdatePdr(
    //             Pdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr2, dedicatedBearer))
    //                 .setPrecedence(dcr2.getPrecedence().value())
    //                 .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
    //                 .setNetworkInstance(UpfConstants::N3NetworkInstance)
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr2))
    //                 .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned"))
    //                 .setOuterHeaderRemoval(Pfcp::OuterHeaderRemoval(Pfcp::OuterHeaderRemovalType::GtpuUdpIp)))
    //         .addUpdatePdr(
    //             PdrHelper::downlink(dcr2)
    //                 .setPrecedence(dcr2.getPrecedence().value())
    //                 .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr2))
    //                 .addQerId(Sft::pfcp::QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
    //                 .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned")))
    //         .addUpdateQer(Qer{QerHelper::getQerNameForDcr(dcr2)}.setUplinkEnabled(false).setDownlinkEnabled(false))
    //         .addRemovePdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr1, dedicatedBearer))
    //         .addRemoveQer(QerHelper::getQerNameForDcr(dcr1)));


   
     upf_.receive(session_,
                     upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));
    upf_.send(session_, upf_.sessionModificationResponse(session_));

    //DedicatedQosFlowsHelper::checkSaccPrintout(session_.getAllBearers(), lcdd_, ue_->supi_->toString());

    banner("release session");
    Procedures::release5gSession();
}


TEST_F(LawfulInterceptionEtsiMultipleQosFlowXnHandover, TC81134_N2HandoverWithAmfChange1)
{


    const auto task     = Li::SessionTasks::Task6;
    // =================================
    // === PDU Session Establishment ===
    // =================================
    addTaskToRedis(task);
    create5gSessionWithPcf(task);
    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    verifyLiT3(task);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));


      banner("install 2 pccrule on one qos flow");
    std::vector<BasicChargingRules::ChargingRule> rules({
        {// rules_[0]
         "DCR1",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         1,  // RatingGroup
         1,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Downlink),
          Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Uplink)}},
        {// rules_[1]
         "DCR2",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         65,  // RatingGroup
         2,   // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.8 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
        {// rules_[2]
         "DCR3",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         8,  // RatingGroup
         3,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.65 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
    });

    auto& dcr1 = rules[0].definition_;
    auto& dcr2 = rules[1].definition_;
    auto& dcr3 = rules[2].definition_;
    // DCR1&DCR2   ----> QCI 3
    auto                bearerQoses = Qos::generateBearerQos({
                       dcr1,
                       dcr2,
    });
    const ProcedureData optionalData{dcr1, dcr2, bearerQoses,Li::SessionTasks::Task6};
     // createQosFlows(optionalData);
    std::vector<Bearer*> bearers = Sft::Util::createQosFlowBearers(optionalData, session_);
    //auto dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);
    Procedures::createQosFlows(optionalData, {session_, bearers});
    

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsBefore("AmfReceiveUpdateSmContextResponseHandoverCommandTransferDirectDataForwardingWithMultipleQos", optionalData, {session_, bearers});

    banner("remove DCR1, update DCR2, install DCR3");

    const auto& qos             = dcr1.getQosInformation();
    auto&       dedicatedBearer = session_.getBearer(*qos->getQosClassIdentifier(), *qos->getArp()->getPriority());

    const auto oldPrecedenceOfDcr2 = dcr2.getPrecedence().value();
    dcr2.setFlowStatus(Avpvalue::FlowStatus::Disabled);
    dcr2.setPrecedence(5);

    std::vector<const Gx::ChargingRuleDefinition*> chargingRuleDefinitions;
    chargingRuleDefinitions.emplace_back(&dcr2);
    chargingRuleDefinitions.emplace_back(&dcr3);

    pcf_.send(session_, N7::chargingRulesToPccRules(
                            N7::MessageBuilder{pcf_.policyNotifyRequest(session_)}.removePccRule(dcr1.getName()),
                            chargingRuleDefinitions));
    

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsAfter("AmfSendUpdateSmContextRequestHandoverRequestAcknowledgeTransferDirectDataForwardingWithMultipleQos", optionalData, {session_, bearers});
    pcf_.receive(session_, pcf_.policyNotifyResponse(session_, TestSupport::StatusCode::NoContent));

    pfcp::MessageBuilder sessionModificationRequest = pfcp::MessageBuilder(upf_.sessionModificationRequest(session_));
    sessionModificationRequest.addRemovePdr(PdrHelper::getPdrDownlinkNameForDcr(dcr1))
        .addUpdatePdr(
            Pdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr2, dedicatedBearer))
                // todo : need to set precedence to 5(new precedence)
                //.setPrecedence(dcr.getPrecedence().value())
                .setPrecedence(2)
                .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
                .setNetworkInstance(UpfConstants::N3NetworkInstance)
                .addQerId(QerHelper::getQerNameForDcr(dcr2))
                .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned"))
                .setOuterHeaderRemoval(Pfcp::OuterHeaderRemoval(Pfcp::OuterHeaderRemovalType::GtpuUdpIp)))
        .addUpdatePdr(
            PdrHelper::downlink(dcr2)
                // todo : need to set precedence to 5(new precedence)
                //.setPrecedence(dcr.getPrecedence().value())
                .setPrecedence(2)
                .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
                .addQerId(QerHelper::getQerNameForDcr(dcr2))
                .addQerId(Sft::pfcp::QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
                .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned")))
        .addCreatePdr(pfcp::PdrHelper::uplink(dcr3, session_.getDefaultBearer())
                          .setPrecedence(*dcr3.getPrecedence())
                          .setNetworkInstance(UpfConstants::N3NetworkInstance)
                          .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
                          .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
                          .addQerId(QerHelper::getQerNameForDcr(dcr3)))
        .addCreateQer(Qer{QerHelper::getQerNameForDcr(dcr3)}
                          .setMbr(TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateDownlink_))
                          .setGbr(TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateDownlink_)));

    //upf_.receive(session_, sessionModificationRequest);


     upf_.receive(session_,
                     upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));
    upf_.send(session_, upf_.sessionModificationResponse(session_));

    // build IEI N1 QoSRules to send
    ::TestSupport::Types::QosRules qosRules;

    // remove a rule
    qosRules.addQosRule(
        ::TestSupport::Types::QosRule()
            .setQosRuleId(session_.amf_.getQosRuleIdForPccRule(*dcr1.getPrecedence()).value())
            .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::DeleteExistingQosRule)
            .setDefaultQosRule(false));

    // update a rule
    qosRules.addQosRule(
        ::TestSupport::Types::QosRule()
            .setQosRuleId(session_.amf_.getQosRuleIdForPccRule(oldPrecedenceOfDcr2).value())
            .setRuleOperationCode(
                ::TestSupport::Types::QosRule::RuleOperationCode::ModifyExistingQosRuleWithoutModifyingPacketFilters)
            .setPrecedence(*dcr2.getPrecedence())
            .setQfi(dedicatedBearer.qfi_)
            .setDefaultQosRule(false)
            .setSegregation(0));

    // create a rule
    qosRules.addQosRule(
        ::TestSupport::Types::QosRule()
            .setQosRuleId(::Compare::MatchAnyValue)  // Creating DCR 3, QRI unknown until created
            .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::CreateNewQosRule)
            .setPrecedence(*dcr3.getPrecedence())
            .setQfi(dedicatedBearer.qfi_)
            .setDefaultQosRule(false)
            .setSegregation(0)
            .addPacketFilter(
                ::TestSupport::Types::PacketFilter()
                    .setIdentifier(::Compare::MatchAnyValue)
                    .setDirection(::TestSupport::Types::PacketFilterDirection::Bidirectional)
                    .addComponent(::TestSupport::Types::PacketFilterComponent::Ipv4RemoteAddress(
                        ::TestSupport::Types::IpAddress("1.1.1.65"),
                        ::TestSupport::Types::IpAddress("255.255.255.255")))
                    .addComponent(::TestSupport::Types::PacketFilterComponent::ProtocolIdentifierNextHeader(
                        ::TestSupport::Types::IpProtocol::Udp))));

    N11::Message& message = amf_.n1n2MessageTransferRequest_ResourceModifyRequest_Basic(session_, qosRules);

    amf_.receive(session_, message);
    amf_.send(session_,
              amf_.n1n2MessageTransferResponse(session_, TestSupport::StatusCode::Ok,
                                               TestSupport::Types::N1N2MessageTransferCause::N1N2TransferInitiated));

    amf_.send(session_, amf_.updateSmContextRequest_ResourceModifyResponseTransfer(session_));
    amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    amf_.send(session_, amf_.updateSmContextRequest_SessionModificationComplete(session_));
    amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    // upf_.receive(
    //     session_,
    //     upf_.sessionModificationRequest(session_)
    //         .addCreatePdr(
    //             pfcp::PdrHelper::downlink(dcr3)
    //                 .setPrecedence(*dcr3.getPrecedence())
    //                 .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
    //                 .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr3)))
    //         .addUpdatePdr(
    //             Pdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr2, dedicatedBearer))
    //                 .setPrecedence(dcr2.getPrecedence().value())
    //                 .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
    //                 .setNetworkInstance(UpfConstants::N3NetworkInstance)
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr2))
    //                 .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned"))
    //                 .setOuterHeaderRemoval(Pfcp::OuterHeaderRemoval(Pfcp::OuterHeaderRemovalType::GtpuUdpIp)))
    //         .addUpdatePdr(
    //             PdrHelper::downlink(dcr2)
    //                 .setPrecedence(dcr2.getPrecedence().value())
    //                 .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr2))
    //                 .addQerId(Sft::pfcp::QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
    //                 .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned")))
    //         .addUpdateQer(Qer{QerHelper::getQerNameForDcr(dcr2)}.setUplinkEnabled(false).setDownlinkEnabled(false))
    //         .addRemovePdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr1, dedicatedBearer))
    //         .addRemoveQer(QerHelper::getQerNameForDcr(dcr1)));


   
     upf_.receive(session_,
                     upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));
    upf_.send(session_, upf_.sessionModificationResponse(session_));

    //DedicatedQosFlowsHelper::checkSaccPrintout(session_.getAllBearers(), lcdd_, ue_->supi_->toString());

    banner("release session");
    Procedures::release5gSession();
}



TEST_F(LawfulInterceptionEtsiMultipleQosFlowXnHandover, TC81134_N2HandoverWithAmfChange2)
{
    const auto task     = Li::SessionTasks::Task6;
    // =================================
    // === PDU Session Establishment ===
    // =================================
    addTaskToRedis(task);
    create5gSessionWithPcf(task);
    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    verifyLiT3(task);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));


      banner("install 2 pccrule on one qos flow");
    std::vector<BasicChargingRules::ChargingRule> rules({
        {// rules_[0]
         "DCR1",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         1,  // RatingGroup
         1,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Downlink),
          Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Uplink)}},
        {// rules_[1]
         "DCR2",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         65,  // RatingGroup
         2,   // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.8 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
        {// rules_[2]
         "DCR3",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         8,  // RatingGroup
         3,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.65 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
    });

    auto& dcr1 = rules[0].definition_;
   // auto& dcr2 = rules[1].definition_;
    auto& dcr3 = rules[2].definition_;
    // DCR1&DCR2   ----> QCI 3
    auto                bearerQoses = Qos::generateBearerQos({
                       dcr1
    });
    const ProcedureData optionalData{dcr1, bearerQoses,Li::SessionTasks::Task6};
     // createQosFlows(optionalData);
    std::vector<Bearer*> bearers = Sft::Util::createQosFlowBearers(optionalData, session_);
    //auto dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);
    Procedures::createQosFlows(optionalData, {session_, bearers});
    

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsBefore("AmfReceiveUpdateSmContextResponseHandoverCommandTransferDirectDataForwardingWithMultipleQos", optionalData, {session_, bearers});

    banner("remove DCR1, update DCR2, install DCR3");

    const auto& qos             = dcr1.getQosInformation();
    auto&       dedicatedBearer = session_.getBearer(*qos->getQosClassIdentifier(), *qos->getArp()->getPriority());



    std::vector<const Gx::ChargingRuleDefinition*> chargingRuleDefinitions;
    chargingRuleDefinitions.emplace_back(&dcr3);

    pcf_.send(session_, N7::chargingRulesToPccRules(
                            N7::MessageBuilder{pcf_.policyNotifyRequest(session_)}.removePccRule(dcr1.getName()),
                            chargingRuleDefinitions));
    

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsAfter("AmfSendUpdateSmContextRequestHandoverRequestAcknowledgeTransferDirectDataForwardingWithMultipleQos", optionalData, {session_, bearers});
    pcf_.receive(session_, pcf_.policyNotifyResponse(session_, TestSupport::StatusCode::NoContent));

    pfcp::MessageBuilder sessionModificationRequest = pfcp::MessageBuilder(upf_.sessionModificationRequest(session_));
    sessionModificationRequest.addRemovePdr(PdrHelper::getPdrDownlinkNameForDcr(dcr1))
        .addCreatePdr(pfcp::PdrHelper::uplink(dcr3, session_.getDefaultBearer())
                          .setPrecedence(*dcr3.getPrecedence())
                          .setNetworkInstance(UpfConstants::N3NetworkInstance)
                          .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
                          .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
                          .addQerId(QerHelper::getQerNameForDcr(dcr3)))
        .addCreateQer(Qer{QerHelper::getQerNameForDcr(dcr3)}
                          .setMbr(TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateDownlink_))
                          .setGbr(TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateDownlink_)));

    //upf_.receive(session_, sessionModificationRequest);


     upf_.receive(session_,
                     upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));
    upf_.send(session_, upf_.sessionModificationResponse(session_));

    // build IEI N1 QoSRules to send
    ::TestSupport::Types::QosRules qosRules;

    // remove a rule
    qosRules.addQosRule(
        ::TestSupport::Types::QosRule()
            .setQosRuleId(session_.amf_.getQosRuleIdForPccRule(*dcr1.getPrecedence()).value())
            .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::DeleteExistingQosRule)
            .setDefaultQosRule(false));
    // create a rule
    qosRules.addQosRule(
        ::TestSupport::Types::QosRule()
            .setQosRuleId(::Compare::MatchAnyValue)  // Creating DCR 3, QRI unknown until created
            .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::CreateNewQosRule)
            .setPrecedence(*dcr3.getPrecedence())
            .setQfi(dedicatedBearer.qfi_)
            .setDefaultQosRule(false)
            .setSegregation(0)
            .addPacketFilter(
                ::TestSupport::Types::PacketFilter()
                    .setIdentifier(::Compare::MatchAnyValue)
                    .setDirection(::TestSupport::Types::PacketFilterDirection::Bidirectional)
                    .addComponent(::TestSupport::Types::PacketFilterComponent::Ipv4RemoteAddress(
                        ::TestSupport::Types::IpAddress("1.1.1.65"),
                        ::TestSupport::Types::IpAddress("255.255.255.255")))
                    .addComponent(::TestSupport::Types::PacketFilterComponent::ProtocolIdentifierNextHeader(
                        ::TestSupport::Types::IpProtocol::Udp))));

    N11::Message& message = amf_.n1n2MessageTransferRequest_ResourceModifyRequest_Basic(session_, qosRules);

    amf_.receive(session_, message);
    amf_.send(session_,
              amf_.n1n2MessageTransferResponse(session_, TestSupport::StatusCode::Ok,
                                               TestSupport::Types::N1N2MessageTransferCause::N1N2TransferInitiated));

    amf_.send(session_, amf_.updateSmContextRequest_ResourceModifyResponseTransfer(session_));
    amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    amf_.send(session_, amf_.updateSmContextRequest_SessionModificationComplete(session_));
    amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    // upf_.receive(
    //     session_,
    //     upf_.sessionModificationRequest(session_)
    //         .addCreatePdr(
    //             pfcp::PdrHelper::downlink(dcr3)
    //                 .setPrecedence(*dcr3.getPrecedence())
    //                 .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
    //                 .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr3)))
    //         .addUpdatePdr(
    //             Pdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr2, dedicatedBearer))
    //                 .setPrecedence(dcr2.getPrecedence().value())
    //                 .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
    //                 .setNetworkInstance(UpfConstants::N3NetworkInstance)
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr2))
    //                 .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned"))
    //                 .setOuterHeaderRemoval(Pfcp::OuterHeaderRemoval(Pfcp::OuterHeaderRemovalType::GtpuUdpIp)))
    //         .addUpdatePdr(
    //             PdrHelper::downlink(dcr2)
    //                 .setPrecedence(dcr2.getPrecedence().value())
    //                 .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr2))
    //                 .addQerId(Sft::pfcp::QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
    //                 .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned")))
    //         .addUpdateQer(Qer{QerHelper::getQerNameForDcr(dcr2)}.setUplinkEnabled(false).setDownlinkEnabled(false))
    //         .addRemovePdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr1, dedicatedBearer))
    //         .addRemoveQer(QerHelper::getQerNameForDcr(dcr1)));


   
     upf_.receive(session_,
                     upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));
    upf_.send(session_, upf_.sessionModificationResponse(session_));

    //DedicatedQosFlowsHelper::checkSaccPrintout(session_.getAllBearers(), lcdd_, ue_->supi_->toString());

    banner("release session");
    Procedures::release5gSession();
}




TEST_F(LawfulInterceptionEtsiMultipleQosFlowXnHandover, TC81134_N2HandoverWithAmfChange3)
{
    const auto task     = Li::SessionTasks::Task6;
    // =================================
    // === PDU Session Establishment ===
    // =================================
    addTaskToRedis(task);
    create5gSessionWithPcf(task);
    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    verifyLiT3(task);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));


      banner("install 2 pccrule on one qos flow");
    std::vector<BasicChargingRules::ChargingRule> rules({
        {// rules_[0]
         "DCR1",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         1,  // RatingGroup
         1,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Downlink),
          Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Uplink)}},
        {// rules_[1]
         "DCR2",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         65,  // RatingGroup
         2,   // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.8 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
        {// rules_[2]
         "DCR3",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(3), 1100_kb, 2100_kb, 100_kb, 200_kb},
         8,  // RatingGroup
         3,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.65 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
    });

    auto& dcr1 = rules[0].definition_;
   // auto& dcr2 = rules[1].definition_;
    auto& dcr3 = rules[2].definition_;
    // DCR1&DCR2   ----> QCI 3
    auto                bearerQoses = Qos::generateBearerQos({
                       dcr1
    });
    const ProcedureData optionalData{dcr1, bearerQoses,Li::SessionTasks::Task6};
     // createQosFlows(optionalData);
    std::vector<Bearer*> bearers = Sft::Util::createQosFlowBearers(optionalData, session_);
    //auto dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);
    Procedures::createQosFlows(optionalData, {session_, bearers});
    

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsBefore("AmfReceiveUpdateSmContextResponseHandoverCommandTransferDirectDataForwardingWithMultipleQos", optionalData, {session_, bearers});

    banner("remove DCR1, update DCR2, install DCR3");

    const auto& qos             = dcr1.getQosInformation();
    auto&       dedicatedBearer = session_.getBearer(*qos->getQosClassIdentifier(), *qos->getArp()->getPriority());



    std::vector<const Gx::ChargingRuleDefinition*> chargingRuleDefinitions;
    chargingRuleDefinitions.emplace_back(&dcr3);

    pcf_.send(session_, N7::chargingRulesToPccRules(
                            N7::MessageBuilder{pcf_.policyNotifyRequest(session_)},
                            chargingRuleDefinitions));
    

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsAfter("AmfSendUpdateSmContextRequestHandoverRequestAcknowledgeTransferDirectDataForwardingWithMultipleQos", optionalData, {session_, bearers});


    //     auto& defaultBearer = session_.getBearerByBearerId(session_.getDefaultBearer().bearerId_);

    // std::vector<Bearer*> defaultDedicatedBearers({&defaultBearer});
    // defaultDedicatedBearers.insert(defaultDedicatedBearers.end(), dedicatedQosBearers.begin(),
    //                                dedicatedQosBearers.end());
    // etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventN2HandoverWithQosFlows(
    //     session_, defaultDedicatedBearers, LiEtsiTest::X2::HandoverState::Completed));


    pcf_.receive(session_, pcf_.policyNotifyResponse(session_, TestSupport::StatusCode::NoContent));

    pfcp::MessageBuilder sessionModificationRequest = pfcp::MessageBuilder(upf_.sessionModificationRequest(session_));
    sessionModificationRequest
        .addCreatePdr(pfcp::PdrHelper::uplink(dcr3, session_.getDefaultBearer())
                          .setPrecedence(*dcr3.getPrecedence())
                          .setNetworkInstance(UpfConstants::N3NetworkInstance)
                          .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
                          .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
                          .addQerId(QerHelper::getQerNameForDcr(dcr3)))
        .addCreateQer(Qer{QerHelper::getQerNameForDcr(dcr3)}
                          .setMbr(TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateDownlink_))
                          .setGbr(TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateDownlink_)));

    //upf_.receive(session_, sessionModificationRequest);


     upf_.receive(session_,
                     upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));
    upf_.send(session_, upf_.sessionModificationResponse(session_));

    // build IEI N1 QoSRules to send
    ::TestSupport::Types::QosRules qosRules;

    // remove a rule
    // qosRules.addQosRule(
    //     ::TestSupport::Types::QosRule()
    //         .setQosRuleId(session_.amf_.getQosRuleIdForPccRule(*dcr1.getPrecedence()).value())
    //         .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::DeleteExistingQosRule)
    //         .setDefaultQosRule(false));
    // create a rule
    qosRules.addQosRule(
        ::TestSupport::Types::QosRule()
            .setQosRuleId(::Compare::MatchAnyValue)  // Creating DCR 3, QRI unknown until created
            .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::CreateNewQosRule)
            .setPrecedence(*dcr3.getPrecedence())
            .setQfi(dedicatedBearer.qfi_)
            .setDefaultQosRule(false)
            .setSegregation(0)
            .addPacketFilter(
                ::TestSupport::Types::PacketFilter()
                    .setIdentifier(::Compare::MatchAnyValue)
                    .setDirection(::TestSupport::Types::PacketFilterDirection::Bidirectional)
                    .addComponent(::TestSupport::Types::PacketFilterComponent::Ipv4RemoteAddress(
                        ::TestSupport::Types::IpAddress("1.1.1.65"),
                        ::TestSupport::Types::IpAddress("255.255.255.255")))
                    .addComponent(::TestSupport::Types::PacketFilterComponent::ProtocolIdentifierNextHeader(
                        ::TestSupport::Types::IpProtocol::Udp))));
        ::TestSupport::Types::QosFlowDescriptions qosFlowDescriptions;
    ::TestSupport::Types::QosFlowDescription  qosFlowDescription;
    qosFlowDescription.setQfi(TestSupport::Types::Qfi{3});
    qosFlowDescription.setOperationCode(
        ::TestSupport::Types::QosFlowDescriptionOperationCode::ModifyExistingQosFlowDescription);
    qosFlowDescription.setEbit(
        ::TestSupport::Types::QosFlowDescriptionEbit::ReplacementOfAllPreviouslyProvidedParameters);
    qosFlowDescription.setFiveQi(::TestSupport::Types::FiveQi{3});
    qosFlowDescription.setMfbrUplink(2200_kb);
    qosFlowDescription.setMfbrDownlink(4200_kb);
    qosFlowDescription.setGfbrUplink(200_kb);
    qosFlowDescription.setGfbrDownlink(400_kb);
    qosFlowDescriptions.addQosFlowDescription(qosFlowDescription);

    N2::NonDynamic5qiDescriptor characteristics;
    characteristics.fiveQi_                            = dedicatedBearer.qos_.qci_.get();
    N2::GbrQosInformation                   gbrQosInfo = N2::GbrQosInformation{{2200_kb, 4200_kb}, {200_kb, 400_kb}};
    const N2::QosFlowLevelQosParameters     parameters{characteristics, dedicatedBearer.qos_.arp_, gbrQosInfo};
    N2::QosFlowAddOrModifyRequestItem       qosFlow{dedicatedBearer.qfi_, parameters, std::nullopt};
    const N2::QosFlowAddOrModifyRequestList qosFlowList{qosFlow};

    N11::Message& message = amf_.n1n2MessageTransferRequest_ResourceModifyRequest_Basic(session_, qosRules,{}, qosFlowList, qosFlowDescriptions);

    amf_.receive(session_, message);
    amf_.send(session_,
              amf_.n1n2MessageTransferResponse(session_, TestSupport::StatusCode::Ok,
                                               TestSupport::Types::N1N2MessageTransferCause::N1N2TransferInitiated));

    amf_.send(session_, amf_.updateSmContextRequest_ResourceModifyResponseTransfer(session_));
    amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    amf_.send(session_, amf_.updateSmContextRequest_SessionModificationComplete(session_));
    amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    // upf_.receive(
    //     session_,
    //     upf_.sessionModificationRequest(session_)
    //         .addCreatePdr(
    //             pfcp::PdrHelper::downlink(dcr3)
    //                 .setPrecedence(*dcr3.getPrecedence())
    //                 .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
    //                 .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr3)))
    //         .addUpdatePdr(
    //             Pdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr2, dedicatedBearer))
    //                 .setPrecedence(dcr2.getPrecedence().value())
    //                 .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
    //                 .setNetworkInstance(UpfConstants::N3NetworkInstance)
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr2))
    //                 .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned"))
    //                 .setOuterHeaderRemoval(Pfcp::OuterHeaderRemoval(Pfcp::OuterHeaderRemovalType::GtpuUdpIp)))
    //         .addUpdatePdr(
    //             PdrHelper::downlink(dcr2)
    //                 .setPrecedence(dcr2.getPrecedence().value())
    //                 .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr2))
    //                 .addQerId(Sft::pfcp::QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
    //                 .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned")))
    //         .addUpdateQer(Qer{QerHelper::getQerNameForDcr(dcr2)}.setUplinkEnabled(false).setDownlinkEnabled(false))
    //         .addRemovePdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr1, dedicatedBearer))
    //         .addRemoveQer(QerHelper::getQerNameForDcr(dcr1)));
    

   
     upf_.receive(session_,
                     upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));
    upf_.send(session_, upf_.sessionModificationResponse(session_));





    

    //DedicatedQosFlowsHelper::checkSaccPrintout(session_.getAllBearers(), lcdd_, ue_->supi_->toString());

    banner("release session");
    Procedures::release5gSession();
}


TEST_F(LawfulInterceptionEtsiMultipleQosFlow, TC81134_N2HandoverWithAmfChange4)
{



      banner("install 2 pccrule on one qos flow");
    std::vector<BasicChargingRules::ChargingRule> rules({
        {// rules_[0]
         "DCR1",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(5), 1100_kb, 2100_kb, 100_kb, 200_kb},
         1,  // RatingGroup
         1,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Downlink),
          Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.1 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Uplink)}},
        {// rules_[1]
         "DCR2",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(5), 1100_kb, 2100_kb, 100_kb, 200_kb},
         65,  // RatingGroup
         2,   // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.8 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
        {// rules_[2]
         "DCR3",
         BearerQos{TestSupport::Types::PreemptionCapability::NotPreempt, 11,
                   TestSupport::Types::PreemptionVulnerability::Preemptable, Qci(5), 1100_kb, 2100_kb, 100_kb, 200_kb},
         8,  // RatingGroup
         3,  // Precedence
         {Gx::FlowInformation()
              .setFlowDescription("permit out 17 from 1.1.1.65 to assigned")
              .setFlowDirection(Avpvalue::FlowDirection::Bidirectional)}},
    });

    auto& dcr1 = rules[0].definition_;
   // auto& dcr2 = rules[1].definition_;
    auto& dcr3 = rules[2].definition_;
    // DCR1&DCR2   ----> QCI 3
    auto                bearerQoses = Qos::generateBearerQos({
                       dcr1
    });
    // const ProcedureData optionalData{dcr1, bearerQoses,Li::SessionTasks::Task6};
    //  // createQosFlows(optionalData);
    // std::vector<Bearer*> bearers = Sft::Util::createQosFlowBearers(optionalData, session_);
    // //auto dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);
    // Procedures::createQosFlows(optionalData, {session_, bearers});
    

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsBefore("AmfReceiveUpdateSmContextResponseHandoverCommandTransferDirectDataForwardingWithMultipleQos", optionalData, {session_, dedicatedQosBearers});

    banner("remove DCR1, update DCR2, install DCR3");

    const auto& qos             = dcr1.getQosInformation();
    auto&       dedicatedBearer = session_.getBearer(*qos->getQosClassIdentifier(), *qos->getArp()->getPriority());



    std::vector<const Gx::ChargingRuleDefinition*> chargingRuleDefinitions;
    chargingRuleDefinitions.emplace_back(&dcr3);

    pcf_.send(session_, N7::chargingRulesToPccRules(
                            N7::MessageBuilder{pcf_.policyNotifyRequest(session_)},
                            chargingRuleDefinitions));
    

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsAfter("AmfSendUpdateSmContextRequestHandoverRequestAcknowledgeTransferDirectDataForwardingWithMultipleQos", optionalData, {session_, dedicatedQosBearers});
    pcf_.receive(session_, pcf_.policyNotifyResponse(session_, TestSupport::StatusCode::NoContent));

    pfcp::MessageBuilder sessionModificationRequest = pfcp::MessageBuilder(upf_.sessionModificationRequest(session_));
    sessionModificationRequest
        .addCreatePdr(pfcp::PdrHelper::uplink(dcr3, session_.getDefaultBearer())
                          .setPrecedence(*dcr3.getPrecedence())
                          .setNetworkInstance(UpfConstants::N3NetworkInstance)
                          .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
                          .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
                          .addQerId(QerHelper::getQerNameForDcr(dcr3)))
        .addCreateQer(Qer{QerHelper::getQerNameForDcr(dcr3)}
                          .setMbr(TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.maxBitrateDownlink_))
                          .setGbr(TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateUplink_),
                                  TestSupport::Types::Kilobits(*rules[2].qos_.guaranteedBitrateDownlink_)));

    //upf_.receive(session_, sessionModificationRequest);


     upf_.receive(session_,
                     upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));
    upf_.send(session_, upf_.sessionModificationResponse(session_));

    // build IEI N1 QoSRules to send
    ::TestSupport::Types::QosRules qosRules;

    // remove a rule
    // qosRules.addQosRule(
    //     ::TestSupport::Types::QosRule()
    //         .setQosRuleId(session_.amf_.getQosRuleIdForPccRule(*dcr1.getPrecedence()).value())
    //         .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::DeleteExistingQosRule)
    //         .setDefaultQosRule(false));
    // create a rule
    qosRules.addQosRule(
        ::TestSupport::Types::QosRule()
            .setQosRuleId(::Compare::MatchAnyValue)  // Creating DCR 3, QRI unknown until created
            .setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::CreateNewQosRule)
            .setPrecedence(*dcr3.getPrecedence())
            .setQfi(dedicatedBearer.qfi_)
            .setDefaultQosRule(false)
            .setSegregation(0)
            .addPacketFilter(
                ::TestSupport::Types::PacketFilter()
                    .setIdentifier(::Compare::MatchAnyValue)
                    .setDirection(::TestSupport::Types::PacketFilterDirection::Bidirectional)
                    .addComponent(::TestSupport::Types::PacketFilterComponent::Ipv4RemoteAddress(
                        ::TestSupport::Types::IpAddress("1.1.1.65"),
                        ::TestSupport::Types::IpAddress("255.255.255.255")))
                    .addComponent(::TestSupport::Types::PacketFilterComponent::ProtocolIdentifierNextHeader(
                        ::TestSupport::Types::IpProtocol::Udp))));
        ::TestSupport::Types::QosFlowDescriptions qosFlowDescriptions;
    ::TestSupport::Types::QosFlowDescription  qosFlowDescription;
    qosFlowDescription.setQfi(TestSupport::Types::Qfi{3});
    qosFlowDescription.setOperationCode(
        ::TestSupport::Types::QosFlowDescriptionOperationCode::ModifyExistingQosFlowDescription);
    qosFlowDescription.setEbit(
        ::TestSupport::Types::QosFlowDescriptionEbit::ReplacementOfAllPreviouslyProvidedParameters);
    qosFlowDescription.setFiveQi(::TestSupport::Types::FiveQi{3});
    qosFlowDescription.setMfbrUplink(2200_kb);
    qosFlowDescription.setMfbrDownlink(4200_kb);
    qosFlowDescription.setGfbrUplink(200_kb);
    qosFlowDescription.setGfbrDownlink(400_kb);
    qosFlowDescriptions.addQosFlowDescription(qosFlowDescription);

    N2::NonDynamic5qiDescriptor characteristics;
    characteristics.fiveQi_                            = dedicatedBearer.qos_.qci_.get();
    N2::GbrQosInformation                   gbrQosInfo = N2::GbrQosInformation{{2200_kb, 4200_kb}, {200_kb, 400_kb}};
    const N2::QosFlowLevelQosParameters     parameters{characteristics, dedicatedBearer.qos_.arp_, gbrQosInfo};
    N2::QosFlowAddOrModifyRequestItem       qosFlow{dedicatedBearer.qfi_, parameters, std::nullopt};
    const N2::QosFlowAddOrModifyRequestList qosFlowList{qosFlow};

    // N11::Message& message = amf_.n1n2MessageTransferRequest_ResourceModifyRequest_Basic(session_, qosRules,{}, qosFlowList, qosFlowDescriptions);

    // amf_.receive(session_, message);
    // amf_.send(session_,
    //           amf_.n1n2MessageTransferResponse(session_, TestSupport::StatusCode::Ok,
    //                                            TestSupport::Types::N1N2MessageTransferCause::N1N2TransferInitiated));

    // amf_.send(session_, amf_.updateSmContextRequest_ResourceModifyResponseTransfer(session_));
    // amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    // amf_.send(session_, amf_.updateSmContextRequest_SessionModificationComplete(session_));
    // amf_.receive(session_, amf_.updateSmContextResponse(session_, TestSupport::StatusCode::NoContent));

    // upf_.receive(
    //     session_,
    //     upf_.sessionModificationRequest(session_)
    //         .addCreatePdr(
    //             pfcp::PdrHelper::downlink(dcr3)
    //                 .setPrecedence(*dcr3.getPrecedence())
    //                 .addSdfFilter(pfcp::SdfFilter().setFlowDescription("permit out 17 from 1.1.1.65 to assigned"))
    //                 .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr3)))
    //         .addUpdatePdr(
    //             Pdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr2, dedicatedBearer))
    //                 .setPrecedence(dcr2.getPrecedence().value())
    //                 .setFarId(FarHelper::getUplinkFarNameForBearer(dedicatedBearer))
    //                 .setNetworkInstance(UpfConstants::N3NetworkInstance)
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr2))
    //                 .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned"))
    //                 .setOuterHeaderRemoval(Pfcp::OuterHeaderRemoval(Pfcp::OuterHeaderRemovalType::GtpuUdpIp)))
    //         .addUpdatePdr(
    //             PdrHelper::downlink(dcr2)
    //                 .setPrecedence(dcr2.getPrecedence().value())
    //                 .setFarId(FarHelper::getDownlinkFarNameForBearer(dedicatedBearer))
    //                 .addQerId(QerHelper::getQerNameForDcr(dcr2))
    //                 .addQerId(Sft::pfcp::QerHelper::getQerNameForDedicatedQosFlow(dedicatedBearer))
    //                 .addSdfFilter(SdfFilter().setFlowDescription(session_, "permit out 17 from 1.1.1.8 to assigned")))
    //         .addUpdateQer(Qer{QerHelper::getQerNameForDcr(dcr2)}.setUplinkEnabled(false).setDownlinkEnabled(false))
    //         .addRemovePdr(PdrHelper::getPdrUplinkNameForDcrAndBearer(dcr1, dedicatedBearer))
    //         .addRemoveQer(QerHelper::getQerNameForDcr(dcr1)));


   
    //  upf_.receive(session_,
    //                  upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));
    // upf_.send(session_, upf_.sessionModificationResponse(session_));

    //DedicatedQosFlowsHelper::checkSaccPrintout(session_.getAllBearers(), lcdd_, ue_->supi_->toString());
        auto& defaultBearer = session_.getBearerByBearerId(session_.getDefaultBearer().bearerId_);

    std::vector<Bearer*> defaultDedicatedBearers({&defaultBearer});
    defaultDedicatedBearers.insert(defaultDedicatedBearers.end(), dedicatedQosBearers.begin(),
                                   dedicatedQosBearers.end());
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventN2HandoverWithQosFlows(
        session_, defaultDedicatedBearers, LiEtsiTest::X2::HandoverState::Completed));
    banner("release session");
std::cout<<"esimnaa 1 \n";
        verifyLiT3(Li::SessionTasks::Task6);


    //Procedures::release5gSession();
}

