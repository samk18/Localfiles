TEST_F(LawfulInterceptionEtsiMultipleQosFlowFailure, TC81134_N2HandoverWithAmfChange)
{
    // ========================
    // === Task added to DB ===
    // ========================
    const auto task = Li::SessionTasks::Task6;
    liRedis_.sendToSubscribers(liRedis_.onKeySetNotification("task", task.iKey_));
    liRedis_.receive(liRedis_.getAllRequest("task", task.iKey_));
    liRedis_.send(liRedis_.getAllTaskReply(session_, {task}));

    SftCore::Logger::banner("Pdu Session establishment");

    Procedures::establish5gSession.runActionsBefore("redisReceiveTargetInfoRequest");
    liRedis_.receive(session_, liRedis_.targetInfoRequest(session_));
    liRedis_.send(liRedis_.targetInfoEtsiReply(session_, {task}));
    Procedures::establish5gSession.runActionsBetween("redisSendTargetInfoResponse",
                                                     "Df2ReceivePduSessionEstablishmentRequest");

    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));
    verifyLiT3(task);

    // Create the non-GBR dedicated flow
    const auto [numSuccessfulFlows, numFailedFlows] = std::make_pair(2,1);
    const auto numChargingRules                     = numSuccessfulFlows + numFailedFlows;
    const auto [definitions, bearerQoses] =
        Sft::Util::createDefinitionsAndQoses(Sft::Util::createNonGbrChargingRules(numChargingRules));
    const ProcedureData optionalData{definitions, bearerQoses, task};
    const auto&         dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);

    banner("createQosFlows");
    Procedures::createQosFlows(optionalData, {session_, dedicatedQosBearers});
    const auto qosRules = Sft::Util::createQosRuleDefinitions(
        optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::CreateNewQosRule);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(session_, dedicatedQosBearers, qosRules,
                                                                               Sft::Li::QosFlowInfo::FullInfo));
    lit3_.dontReceive();


    for(int i=2;i<3;i++)
    {
        dedicatedQosBearers[i]->isFailed = true;
    }

    // Setup which flows that should be successful or failed.
    std::vector<Bearer*> associatedQosFlowList    = {};
    std::vector<Bearer*> qosFlowFailedToSetupList = {};

   // associatedQosFlowList.push_back(&session_.getDefaultBearer());

    std::copy(dedicatedQosBearers.begin(), dedicatedQosBearers.begin() + numSuccessfulFlows,
              std::back_inserter(associatedQosFlowList));
    std::copy(dedicatedQosBearers.begin() + numSuccessfulFlows, dedicatedQosBearers.end(),
              std::back_inserter(qosFlowFailedToSetupList));

   
   banner("Start N2 HO With Amf");
    // Procedures::n2HandoverWithoutAmfChange.runActionsBefore(
    //     "amfReceiveUpdateSmContextResponseHandoverCommandTransfer", optionalData,
    //     {session_, associatedQosFlowList});

    // amf_.send(session_, amf_.updateSmContextRequest_HandoverRequestAcknowledgeTransfer(session_));

    // Procedures::n2HandoverWithoutAmfChange.runActionsBetween(
    //     "amfReceiveUpdateSmContextResponseHandoverCommandTransfer",
    //     "UpfReceiveSessionModificationRequest_DownlinkFARUpdate", optionalData, {session_, associatedQosFlowList});
    // std::cout<<"hello world 1\n";
    // upf_.receive(session_,
    //              upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));

    // Procedures::n2HandoverWithoutAmfChange.runActionsAfter("UpfReceiveSessionModificationRequest_DownlinkFARUpdate",
    //                                                     optionalData, {session_, associatedQosFlowList});

     Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsBefore("UpfReceiveSessionModificationRequest_DownlinkFARUpdateWithMultipleQos",optionalData, {session_, associatedQosFlowList});

      upf_.receive(session_,
                 upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsAfter("UpfReceiveSessionModificationRequest_DownlinkFARUpdateWithMultipleQos",optionalData, {session_, associatedQosFlowList});

  associatedQosFlowList.insert(associatedQosFlowList.begin(), &session_.getDefaultBearer());
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventN2HandoverWithQosFlows(
        session_, associatedQosFlowList, LiEtsiTest::X2::HandoverState::Completed));

session_.amf_.upCnxState_ = TestSupport::Types::UpCnxState::Deactivated;
           banner("SMF initiate delete flows for failed QoS flows");
        auto                definitionsGbr = std::vector(definitions.begin() + numSuccessfulFlows, definitions.end());
        auto                bearerQosesGbr = std::vector(bearerQoses.begin() + numSuccessfulFlows, bearerQoses.end());
        const ProcedureData optionalDataGbr{definitionsGbr, bearerQosesGbr, task};



//  std::cout<<"yes failed12\n";

Procedures::smfDeleteQosFlows.runActionsBetween("AmfSendEbiReleaseResponse", "AmfSendUpdateSmContextRequest_N2ResourceModifyResponseTransfer", optionalDataGbr, {session_, qosFlowFailedToSetupList});

 Procedures::deleteQosFlows.runActionsBetween("AmfSendN1n2MessageTransferResponse","UpfSessionModificationRequestUplinkRemoval",optionalDataGbr, {session_, qosFlowFailedToSetupList});
// //Procedures::smfDeleteQosFlows.runActionsAfter("AmfSendN1n2MessageTransferResponse", optionalDataGbr, {session_, qosFlowFailedToSetupList});
//  //Procedures::anDeleteQosFlows.runActionsAfter("AmfReceiveUpdateSmContextN1Response_NoContent", optionalDataGbr, {session_, qosFlowFailedToSetupList});
 Procedures::smfDeleteQosFlows.runActionsAfter("AmfReceiveUpdateSmContextN1Response_NoContent", optionalDataGbr, {session_, qosFlowFailedToSetupList});

//  etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventN2HandoverWithQosFlows(
//         session_, associatedQosFlowList, LiEtsiTest::X2::HandoverState::Completed));
// ::TestSupport::Types::QosRules pccRules;
//               QosRule qosRule{};
//         qosRule.setQosRuleId(::Compare::MatchAnyValue);
//         qosRule.setRuleOperationCode(::TestSupport::Types::QosRule::RuleOperationCode::DeleteExistingQosRule);
//         //qosRule.setQfi(::Compare::MatchAnyValue);
//         qosRule.setDefaultQosRule(false);
// pccRules.addQosRule(qosRule);

//         QosFlowDescription qosFlowDescription{};
//         qosFlowDescription.setQfi(::Compare::MatchAnyValue);
// qosFlowDescription.setEbit(QosFlowDescriptionEbit::ParametersListIsNotIncluded);
//         qosFlowDescription.setOperationCode(QosFlowDescriptionOperationCode::DeleteExistingQosFlowDescription);
// QosFlowDescriptions               qosFlowDescriptions{};
// qosFlowDescriptions.addQosFlowDescription(qosFlowDescription);

//  N11::Message& n11Message = amf_.n1n2MessageTransferRequest_ResourceModifyRequest_Basic(session_, pccRules, {}, {},
//                                                                            qosFlowDescriptions);



//  amf_.receive(session_, n11Message);



// amf_.send(session_,
//               amf_.n1n2MessageTransferResponse(session_, TestSupport::StatusCode::Ok,
//                                                TestSupport::Types::N1N2MessageTransferCause::N1N2TransferInitiated));




// Procedures::deleteQosFlows.runActionsBetween("AmfSendN1n2MessageTransferResponse","UpfSessionModificationRequestUplinkRemoval",optionalDataGbr, {session_, qosFlowFailedToSetupList});

// Procedures::anDeleteQosFlows.runActionsAfter("AmfReceiveUpdateSmContextN1Response_NoContent", optionalDataGbr, {session_, qosFlowFailedToSetupList});




//     etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventN2HandoverWithQosFlows(
//         session_, associatedQosFlowList, LiEtsiTest::X2::HandoverState::Completed));

   //  amf_.receive(session_, amf_.ebiAssignmentReleaseEbiRequest(session_, qosFlowFailedToSetupList));
    //  Procedures::deleteQosFlows.runActionsBefore("UpfSessionModificationRequestDownlinkRemoval",optionalDataGbr, {session_, qosFlowFailedToSetupList});
// auto& n1n2MessageWithoutN1 = amf_.n1n2MessageTransferRequest_ResourceModifyRequest(session_);

  
//     TestSupport::Types::Qci qci = session_.getDefaultBearer().qos_.qci_;
//     TestSupport::Types::Arp arp = TestSupport::Types::Arp(session_.getDefaultBearer().qos_.arp_);


//      auto& rescourceModifyRequest = amf_.n1n2MessageTransferRequest_ResourceModifyRequest_Create_DeleteQosFlow(
//         session_, {}, qosFlowFailedToSetupList, optionalDataGbr.getChargingRuleDefinitions(), qci, arp);
//     amf_.receive(session_, rescourceModifyRequest);


//     amf_.receive(session_, n1n2MessageWithoutN1);
    // auto& defaultBearer = session_.getBearerByBearerId(session_.getDefaultBearer().bearerId_);

    // std::vector<Bearer*> defaultDedicatedBearers({&defaultBearer});
    // defaultDedicatedBearers.insert(defaultDedicatedBearers.end(), dedicatedQosBearers.begin(),
    //                                dedicatedQosBearers.end());
    // etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventN2HandoverWithQosFlows(
    //     session_, defaultDedicatedBearers, LiEtsiTest::X2::HandoverState::Completed));

    Procedures::release5gSession();
 //   etsiDf2_.receive(etsiDf2_.pduSessionReleaseInterceptEvent(session_));

}





//*=============================================================================
// test case: TC81637_N2HandoverWithoutAmfChangeFailedDedicatedQosFlows
//
//#Description#
//    Verify that when a dedicated QoS flow is included in the QoS Flow Failed
//    Setup List during N2 Handover, the SMF initiated a PDU session modification to remove failed
//    QoS flows, that triggers a PDU Session modification.
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_P(LawfulInterceptionEtsiMultipleQosFlowFailure, TC81637_N2HandoverWithoutAmfChangeFailedDedicatedQosFlows)
{
    // ========================
    // === Task added to DB ===
    // ========================
    const auto task = Li::SessionTasks::Task6;
    liRedis_.sendToSubscribers(liRedis_.onKeySetNotification("task", task.iKey_));
    liRedis_.receive(liRedis_.getAllRequest("task", task.iKey_));
    liRedis_.send(liRedis_.getAllTaskReply(session_, {task}));

    SftCore::Logger::banner("Pdu Session establishment");

    Procedures::establish5gSession.runActionsBefore("redisReceiveTargetInfoRequest");
    liRedis_.receive(session_, liRedis_.targetInfoRequest(session_));
    liRedis_.send(liRedis_.targetInfoEtsiReply(session_, {task}));
    Procedures::establish5gSession.runActionsBetween("redisSendTargetInfoResponse",
                                                     "Df2ReceivePduSessionEstablishmentRequest");

    etsiDf2_.receive(etsiDf2_.pduSessionEstablishInterceptEvent(session_));
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithDefaultQosInActiveList(session_));
    verifyLiT3(task);

    // Create the non-GBR dedicated flow
    const auto [numSuccessfulFlows, numFailedFlows] = GetParam();
    const auto numChargingRules                     = numSuccessfulFlows + numFailedFlows;
    const auto [definitions, bearerQoses] =
        Sft::Util::createDefinitionsAndQoses(Sft::Util::createNonGbrChargingRules(numChargingRules));
    const ProcedureData optionalData{definitions, bearerQoses, task};
    const auto&         dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);

    banner("createQosFlows");
    Procedures::createQosFlows(optionalData, {session_, dedicatedQosBearers});
    const auto qosRules = Sft::Util::createQosRuleDefinitions(
        optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::CreateNewQosRule);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(session_, dedicatedQosBearers, qosRules,
                                                                               Sft::Li::QosFlowInfo::FullInfo));
    lit3_.dontReceive();

    // Setup which flows that should be successful or failed.
    std::vector<Bearer*> associatedQosFlowList    = {};
    std::vector<Bearer*> qosFlowFailedToSetupList = {};

    associatedQosFlowList.push_back(&session_.getDefaultBearer());

    std::copy(dedicatedQosBearers.begin(), dedicatedQosBearers.begin() + numSuccessfulFlows,
              std::back_inserter(associatedQosFlowList));
    std::copy(dedicatedQosBearers.begin() + numSuccessfulFlows, dedicatedQosBearers.end(),
              std::back_inserter(qosFlowFailedToSetupList));

    ASSERT_GE(associatedQosFlowList.size(), 1);  // Mandatory IE and must have at least one bearer.

    for(int i = numSuccessfulFlows;i < numChargingRules; i++)
    {
        dedicatedQosBearers[i]->isFailed = true;
    }


    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsBefore("UpfReceiveSessionModificationRequest_DownlinkFARUpdateWithMultipleQos",optionalData, {session_, associatedQosFlowList});

    upf_.receive(session_,
                 upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsAfter("UpfReceiveSessionModificationRequest_DownlinkFARUpdateWithMultipleQos",optionalData, {session_, associatedQosFlowList});
    
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventN2HandoverWithQosFlows(
        session_, associatedQosFlowList, LiEtsiTest::X2::HandoverState::Completed));

    if (numFailedFlows > 0)
    {
        banner("SMF initiate delete flows for failed QoS flows");
        auto                definitionsGbr = std::vector(definitions.begin() + numSuccessfulFlows, definitions.end());
        auto                bearerQosesGbr = std::vector(bearerQoses.begin() + numSuccessfulFlows, bearerQoses.end());
        const ProcedureData optionalDataGbr{definitionsGbr, bearerQosesGbr, task};
        session_.amf_.upCnxState_ = TestSupport::Types::UpCnxState::Deactivated;

        Procedures::deleteQosFlows.runActionsBetween("UpfSessionModificationResponseDownlinkRemoval","UpfSessionModificationRequestUplinkRemoval",optionalDataGbr, {session_, qosFlowFailedToSetupList});
        Procedures::smfDeleteQosFlows.runActionsAfter("AmfReceiveUpdateSmContextN1Response_NoContent", optionalDataGbr, {session_, qosFlowFailedToSetupList});
        verifyLiT3(task);
        const auto deleteQosRules = Sft::Util::createQosRuleDefinitions(
            optionalData, qosFlowFailedToSetupList, TestSupport::Types::QosRuleRuleOperationCode::DeleteExistingQosRule);
        etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventDeleteQosFlow(session_, qosFlowFailedToSetupList,
                                                                                    deleteQosRules));
    }

    // =======================
    // === Session Release ===
    // =======================
    SftCore::Logger::banner("Pdu Session Release");
    Procedures::release5gSession.runActionsBefore("Df2ReceivePduSessionReleaseEventNoUeLoc");
    etsiDf2_.receive(etsiDf2_.pduSessionReleaseInterceptEvent(session_)); 
}



//*=============================================================================
// test case: TC81637_N2HandoverWithoutAmfChangeFailedDedicatedQosFlows
//
//#Description#
//    Verify that when a dedicated QoS flow is included in the QoS Flow Failed
//    Setup List during N2 Handover, the SMF initiated a PDU session modification to remove failed
//    QoS flows, that triggers a PDU Session modification.
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_P(LawfulInterceptionEtsiMultipleQosFlowFailure, TC81637_N2HandoverWithoutAmfChangeFailedDedicatedQosFlows)
{
    // ========================
    // === Task added to DB ===
    // ========================
    const auto task = Li::SessionTasks::Task6;
    addTaskToRedis(task);

    SftCore::Logger::banner("Pdu Session establishment");
    Procedures::establish5gSession({task});

    // Create the non-GBR dedicated flow
    const auto [numSuccessfulFlows, numFailedFlows] = GetParam();
    const auto numChargingRules                     = numSuccessfulFlows + numFailedFlows;
    const auto [definitions, bearerQoses] =
        Sft::Util::createDefinitionsAndQoses(Sft::Util::createNonGbrChargingRules(numChargingRules));
    const ProcedureData optionalData{definitions, bearerQoses, task};
    const auto&         dedicatedQosBearers = Sft::Util::createQosFlowBearers(optionalData);

    banner("createQosFlows");
    Procedures::createQosFlows(optionalData, {session_, dedicatedQosBearers});
    const auto qosRules = Sft::Util::createQosRuleDefinitions(
        optionalData, dedicatedQosBearers, TestSupport::Types::QosRuleRuleOperationCode::CreateNewQosRule);
    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventWithQosFlows(session_, dedicatedQosBearers, qosRules,
                                                                               Sft::Li::QosFlowInfo::FullInfo));
    lit3_.dontReceive();

    // Setup which flows that should be successful or failed.
    std::vector<Bearer*> associatedQosFlowList    = {};
    std::vector<Bearer*> qosFlowFailedToSetupList = {};

    associatedQosFlowList.push_back(&session_.getDefaultBearer());

    std::copy(dedicatedQosBearers.begin(), dedicatedQosBearers.begin() + numSuccessfulFlows,
              std::back_inserter(associatedQosFlowList));
    std::copy(dedicatedQosBearers.begin() + numSuccessfulFlows, dedicatedQosBearers.end(),
              std::back_inserter(qosFlowFailedToSetupList));

    ASSERT_GE(associatedQosFlowList.size(), 1);  // Mandatory IE and must have at least one bearer.

    for (int i = numSuccessfulFlows; i < numChargingRules; i++)
    {
        dedicatedQosBearers[i]->isFailed = true;
    }

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsBefore(
        "UpfReceiveSessionModificationRequest_DownlinkFARUpdateWithMultipleQos", optionalData,
        {session_, associatedQosFlowList});

    upf_.receive(session_,
                 upf_.sessionModificationRequest(session_).setVerificationLevel(VerificationLevel::OnlySpecified));

    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsAfter(
        "UpfReceiveSessionModificationRequest_DownlinkFARUpdateWithMultipleQos", optionalData,
        {session_, associatedQosFlowList});

    etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventN2HandoverWithQosFlows(
        session_, associatedQosFlowList, LiEtsiTest::X2::HandoverState::Completed));

    if (numFailedFlows > 0)
    {
        banner("SMF initiate delete flows for failed QoS flows");
        auto                definitionsGbr = std::vector(definitions.begin() + numSuccessfulFlows, definitions.end());
        auto                bearerQosesGbr = std::vector(bearerQoses.begin() + numSuccessfulFlows, bearerQoses.end());
        const ProcedureData optionalDataGbr{definitionsGbr, bearerQosesGbr, task};
        session_.amf_.upCnxState_ = TestSupport::Types::UpCnxState::Deactivated;

        Procedures::deleteQosFlows.runActionsBetween("UpfSessionModificationResponseDownlinkRemoval",
                                                     "UpfSessionModificationRequestUplinkRemoval", optionalDataGbr,
                                                     {session_, qosFlowFailedToSetupList});
        Procedures::smfDeleteQosFlows.runActionsAfter("AmfReceiveUpdateSmContextN1Response_NoContent", optionalDataGbr,
                                                      {session_, qosFlowFailedToSetupList});
        verifyLiT3(task);
        const auto deleteQosRules =
            Sft::Util::createQosRuleDefinitions(optionalData, qosFlowFailedToSetupList,
                                                TestSupport::Types::QosRuleRuleOperationCode::DeleteExistingQosRule);
        etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventDeleteQosFlow(session_, qosFlowFailedToSetupList,
                                                                                    deleteQosRules));
    }

    // =======================
    // === Session Release ===
    // =======================
    SftCore::Logger::banner("Pdu Session Release");
    Procedures::release5gSession.runActionsBefore("Df2ReceivePduSessionReleaseEventNoUeLoc");
    etsiDf2_.receive(etsiDf2_.pduSessionReleaseInterceptEvent(session_));
}


//*=============================================================================
// test case: TC81398_N2HandoverWithOutAmfChangeWithInstallPccRules
//
//#Description#
//     Verify N2Handover without Amf change with multiple Qos flows where
//     PCF provisions new PCC rules/session rules and SMF sends PDU session
//     modification event for the created pcc rules after Handover.
//
//#Feature#
//    PCPB-20230
//
//#Requirement#
//    REQ21354
//=============================================================================*
TEST_F(LawfulInterceptionEtsiMultipleQosFlow, TC81398_N2HandoverWithOutAmfChangeWithInstallPccRules)
{
    banner("Start N2 HO DirectDataForwarding");
    Procedures::n2HandoverDirectDataForwardingWithMultipleQos.runActionsBefore("AmfSendUpdateSmContextRequestHandoverComplete", optionalData, {session_, dedicatedQosBearers});

    auto& amfSmContextUpdateRequest = amf_.updateSmContextRequest(session_);
    amfSmContextUpdateRequest.getJsonBuilder().setHoState(TestSupport::Types::HoState::Cancelled).build();

    amf_.send(session_, amfSmContextUpdateRequest);
   

    // auto& defaultBearer = session_.getBearerByBearerId(session_.getDefaultBearer().bearerId_);

    // std::vector<Bearer*> movedBearers({&defaultBearer});
    // movedBearers.insert(movedBearers.end(), dedicatedQosBearers.begin(), dedicatedQosBearers.end());
    // etsiDf2_.receive(etsiDf2_.pduSessionModificationInterceptEventN2HandoverWithQosFlows(
    //     session_, movedBearers, LiEtsiTest::X2::HandoverState::Completed));
}
