// ======================================================================
// \title  ComponentA.cpp
// \author shahab
// \brief  cpp file for ComponentA component implementation class
// ======================================================================

#include "FprimeGenericHubReference/Components/ComponentA/ComponentA.hpp"
#include <Fw/Com/ComBuffer.hpp>
#include <Svc/GenericHub/GenericHub.hpp>

namespace FprimeGenericHubReference {

const U32 VERIFY_WORD0 = 0xf754a98b; 
const U32 VERIFY_WORD1 = 0xcb0431d2;

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ComponentA ::ComponentA(const char* const compName) : ComponentAComponentBase(compName) {
    m_ghCounter = 0;
    tlmId = this->getIdBase() + 1;
    evrId = this->getIdBase() + 2;
    tlmTime.set(693710,498);
    evrTime.set(589756,953);

}

ComponentA ::~ComponentA() {}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void ComponentA ::send_serialData() {
    Fw::ComBuffer serBuf;
    serBuf.resetSer();

    //Set serialization buffer for serOut
    serBuf.serializeFrom(VERIFY_WORD0);
    this->serOut_out(0,serBuf);
}

void ComponentA ::send_buffer() {
    //Set for buffer_out
    U32 max_random_size = 500;
    U32 sampleSendData[2];
    sampleSendData[0] = VERIFY_WORD0; 
    sampleSendData[1] = VERIFY_WORD1; 

    this->m_buffer.set(m_data_store, sizeof(m_data_store));
    auto serializer = m_buffer.getSerializer();

    Fw::SerializeStatus status;
    status = serializer.serializeFrom(sampleSendData[0]);
    FW_ASSERT(status == Fw::FW_SERIALIZE_OK, sampleSendData[0]);

    status = serializer.serializeFrom(sampleSendData[1]);
    FW_ASSERT(status == Fw::FW_SERIALIZE_OK, sampleSendData[1]);

    this->m_buffer.setSize(max_random_size);
    this->opBuff_out(0,m_buffer);
}

void ComponentA ::send_tlm(U32 count) {
    Fw::TlmBuffer tlmValue;
    Fw::SerializeStatus status = Fw::FW_SERIALIZE_OK;

    //Telemetry (channel) Out for this deployment (DeploymentA)
    this->tlmWrite_GhCounter(count);

    //Telemetry (channel) Out for DeploymentB
    status = tlmValue.serializeFrom(VERIFY_WORD1);
    FW_ASSERT(status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(status) );
    this->telemOut_out(0, this->tlmId, this->tlmTime, tlmValue);

}

void ComponentA ::send_event(U32 count) {
    Fw::SerializeStatus status = Fw::FW_SERIALIZE_OK;
    
    Fw::LogBuffer logBuff;

    //Event Out for DeploymentB
    status = logBuff.serializeFrom(VERIFY_WORD0);
    FW_ASSERT(status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(status) );
    this->eventOut_out(0, this->evrId, this->evrTime, Fw::LogSeverity::ACTIVITY_LO, logBuff);

}

void ComponentA ::HubMessageTest_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 hubMsgType) {

   
    ++this->m_ghCounter;

    //Event Out for this deployment (DeploymentA)
    this->log_ACTIVITY_LO_HubTest(this->m_ghCounter); 

    // Values of Svc::GenericHub_HubType select one message kind; NUM_CONSTANTS sends all of them
    switch (hubMsgType) {

    /* Test Serial ports -- serialIn/serialOut  */
    case Svc::GenericHub_HubType::HUB_TYPE_PORT:
        this->send_serialData(); 
        break;

     /* Test Buffer ports -- bufferIn/bufferOut */
    case Svc::GenericHub_HubType::HUB_TYPE_BUFFER:
        this->send_buffer(); 
        break;

    /* Test Event (EVRs) ports -- EventIn/EventOut */
    case Svc::GenericHub_HubType::HUB_TYPE_EVENT:
        this->send_event(this->m_ghCounter);
        break;

    /* Test telemetery (channels) ports -- TlmIn/TlmOut */
    case Svc::GenericHub_HubType::HUB_TYPE_CHANNEL:
        this->send_tlm(this->m_ghCounter);
        break; 

    case Svc::GenericHub_HubType::NUM_CONSTANTS:
        this->send_serialData();
        this->send_buffer();
        this->send_tlm(this->m_ghCounter);
        this->send_event(this->m_ghCounter);
        break;

    case Svc::GenericHub_HubType::HUB_TYPE_CMD_DISP:
    case Svc::GenericHub_HubType::HUB_TYPE_CMD_RESP:
        //do nothing
        break;
    }

    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void ComponentA ::serIn_handler(
          FwIndexType portNum, //!< The port number
          Fw::LinearBufferBase& serBuffer
      ) {

        Fw::ComBuffer serBufOut;
        serBufOut.resetSer();
        U32 val; 

        serBuffer.deserializeTo(val);

        if (val == VERIFY_WORD0) {
            this->log_ACTIVITY_LO_SerDataVerif(); 
        } else { 
            this->log_WARNING_LO_SerDataVerifFail(); 
        }        
}

//! Handler for input port telemIn
void ComponentA ::telemIn_handler(
        FwIndexType portNum, //!< The port number
        FwChanIdType id, //!< Telemetry Channel ID
        Fw::Time& timeTag, //!< Time Tag
        Fw::TlmBuffer& tlmBuf //!< Buffer containing serialized telemetry value
      ) {
        U32 val; 
        Fw::SerializeStatus status = Fw::FW_SERIALIZE_OK;
        
        status = tlmBuf.deserializeTo(val);
        FW_ASSERT(status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(status) );
        this->tlmWrite_verifyWord1(val);

        if (val == VERIFY_WORD1 && id==this->tlmId && 
            timeTag.getSeconds() == this->tlmTime.getSeconds() && 
            timeTag.getUSeconds() == this->tlmTime.getUSeconds()) {
            this->log_ACTIVITY_LO_TlmDataVerif();
        } else { 
            this->log_WARNING_LO_TlmDataVerifFail();
        }
}

void ComponentA ::eventIn_handler(
          FwIndexType portNum, //!< The port number
          FwEventIdType id, //!< Log ID
          Fw::Time& timeTag, //!< Time Tag
          const Fw::LogSeverity& severity, //!< The severity argument
          Fw::LogBuffer& args //!< Buffer containing serialized log entry
    ) {

        Fw::SerializeStatus status = Fw::FW_SERIALIZE_OK;
        U32 val; 

        //Event received from DeploymentB
        status = args.deserializeTo(val);
        FW_ASSERT(status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(status) );

        this->tlmWrite_verifyWord0(val);
        if (val == VERIFY_WORD0 && id==this->evrId && 
            timeTag.getSeconds() == this->evrTime.getSeconds() && 
            timeTag.getUSeconds() == this->evrTime.getUSeconds()) {
            this->log_ACTIVITY_LO_EvrDataVerif();
        } else { 
            this->log_WARNING_LO_EvrDataVerifFail();
        }
}

void ComponentA ::bufIn_handler(
          FwIndexType portNum, //!< The port number
          Fw::Buffer& fwBuffer //!< The buffer
) {
    U32 sampleRcvData[2]; 
    Fw::SerializeStatus status = Fw::FW_SERIALIZE_OK;
    FW_ASSERT(fwBuffer.isValid());
    auto deserializer = fwBuffer.getDeserializer();

    status = deserializer.deserializeTo(sampleRcvData[0]);
    status = deserializer.deserializeTo(sampleRcvData[1]);


    if (sampleRcvData[0] == VERIFY_WORD0 && sampleRcvData[1] == VERIFY_WORD1) {
        this->log_ACTIVITY_LO_BufDataVerif(); 
    } else { 
        this->log_WARNING_LO_BufDataVerifFail();
    }
}

void ComponentA ::bufRet_handler(
          FwIndexType portNum, //!< The port number
          Fw::Buffer& fwBuffer //!< The buffer
      ) {

        
}

}  // namespace FprimeGenericHubReference
