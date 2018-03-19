#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <mutex>

#include "mcBoard_E-1608.h"

static void readThreadC(void *pPvt)
{
    mcE_1608 *pmcE_1608 = (mcE_1608*) pPvt;
    pmcE_1608->readThread();
}

mcE_1608::mcE_1608(const char *address)
  : mcBoard(address)
{
    strcpy(boardName_, "E-1608");
    biBoardType_    = E1608_PID;
    biNumADCChans_  = 8;
    biADCRes_       = 16;
    biNumDACChans_  = 2;
    biDACRes_       = 16;
    biNumTempChans_ = 0;
    biDInNumDevs_   = 1;
    diDevType_      = AUXPORT;
    diInMask_       = 0;
    diOutMask_      = 0;
    diNumBits_      = 8;
    
    aiScanAcquiring_ = false;
    aiScanCurrentPoint_ = 0;
    
    // Open Ethernet socket
    deviceInfo_.device.connectCode = 0x0;   // default connect code
    deviceInfo_.device.frameID = 0;         // zero out the frameID
    deviceInfo_.queue[0] = 0;               // set count in gain queue to zero
    deviceInfo_.timeout = 1000;             // set default timeout to 1000 ms.
    deviceInfo_.device.Address.sin_family = AF_INET;
    deviceInfo_.device.Address.sin_port = htons(COMMAND_PORT);
    deviceInfo_.device.Address.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, address_, &deviceInfo_.device.Address.sin_addr) == 0) {
        printf("Improper destination address.\n");
        return;
    }

    if ((deviceInfo_.device.sock = openDevice(inet_addr(inet_ntoa(deviceInfo_.device.Address.sin_addr)),
                                              deviceInfo_.device.connectCode)) < 0) {
        printf("Error opening socket\n");
        return;
    }
    buildGainTableAIn_E1608(&deviceInfo_);
    buildGainTableAOut_E1608(&deviceInfo_);
    
    pReadThread_ = new std::thread(readThreadC, this);
    pReadMutex_ = new std::mutex();
}


void mcE_1608::readThread()
{
    struct timespec ts;
    int bytesReceived = 0;
    int sock;
    int totalBytesToRead;
    int totalBytesReceived;
    int currentChannel;
    int channel;
    int range;
    uint16_t *rawBuffer=0;
    uint16_t rawData;
    int correctedData;
    int timeout = deviceInfo_.timeout;

    ts.tv_sec = 0;
    ts.tv_nsec = 10000000;  // 10 ms
    pReadMutex_->lock();
printf("ReadThread entry\n");
    while (1) {
        pReadMutex_->unlock();
        nanosleep(&ts, 0);
        pReadMutex_->lock();
        if (!aiScanAcquiring_) continue;
        sock = deviceInfo_.device.scan_sock;
        if (sock < 0) {
            printf("Error no scan sock\n");
            continue;
        }
        totalBytesToRead = aiScanTotalElements_*2;
        totalBytesReceived = 0;
        aiScanCurrentPoint_ = 0;
        aiScanCurrentIndex_ = 0;
        currentChannel = 0;
        if (rawBuffer) free (rawBuffer);
        rawBuffer = (uint16_t *)calloc(aiScanTotalElements_, sizeof(uint16_t));

        do {
            pReadMutex_->unlock();
            bytesReceived = receiveMessage(sock, &rawBuffer[totalBytesReceived/2], totalBytesToRead - totalBytesReceived, timeout);
            pReadMutex_->lock();
            if (bytesReceived <= 0) {  // error
                printf("Error in mcE_1608::readThread: totalBytesReceived = %d totalBytesToRead = %d \n", totalBytesReceived, totalBytesToRead);
                continue;
            }
            totalBytesReceived += bytesReceived;
            int newPoints = bytesReceived / 2;
            for (int i=0; i<newPoints; i++) {
                rawData = rawBuffer[aiScanCurrentPoint_];
                channel = deviceInfo_.queue[2*currentChannel+1];
                range = deviceInfo_.queue[2*currentChannel+2];
                if (channel < DF) {  // single ended
                    correctedData = rint(rawData*deviceInfo_.table_AInSE[range].slope + deviceInfo_.table_AInSE[range].intercept);
                } else {  // differential
                    correctedData = rint(rawData*deviceInfo_.table_AInDF[range].slope + deviceInfo_.table_AInDF[range].intercept);
                }
                if (correctedData > 65536) {
                    correctedData = 65535;  // max value
                } else if (correctedData < 0) {
                    correctedData = 0;
                }
                if (aiScanIsScaled_) {
                    aiScanScaledBuffer_[aiScanCurrentPoint_] = volts_E1608(correctedData, range);
                } else {
                    aiScanUnscaledBuffer_[aiScanCurrentPoint_] = correctedData;
                }
                aiScanCurrentPoint_++;
                currentChannel ++;
                if (currentChannel >= aiScanNumChans_) {
                    currentChannel = 0;
                    aiScanCurrentIndex_++;
                }
            };
        } while (totalBytesReceived < totalBytesToRead);
        aiScanAcquiring_ = false;
        close(sock);
    }
}

int mcE_1608::cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal) {
    switch (InfoType) {
    case BOARDINFO:
        switch (ConfigItem) {

        default:
            printf("mcBoardE_T-1608::setConfig error unknown ConfigItem %d\n", ConfigItem);
            return BADCONFIGITEM;
            break;
    }
    break;
    
    default:
        printf("mcBoardE_T-1608::setConfig error unknown InfoType %d\n", InfoType);
        return BADCONFIGTYPE;
    break;
    }
    return NOERRORS;
}

int mcE_1608::cbGetIOStatus(short *Status, long *CurCount, long *CurIndex, int FunctionType)
{
    if (FunctionType == AIFUNCTION) {
        pReadMutex_->lock();
        *Status = aiScanAcquiring_ ? 1 : 0;
        *CurCount = aiScanCurrentPoint_;
        *CurIndex = aiScanCurrentIndex_;
        pReadMutex_->unlock();
    }
    return NOERRORS;

}

static int gainToRange(int Gain)
{
    switch (Gain) {
      case BIP1VOLTS:
        return BP_1V;
      case BIP2VOLTS:
        return BP_2V;
      case BIP5VOLTS:
        return BP_5V;
      case BIP10VOLTS:
        return BP_10V;
      default:
        printf("Unsupported Gain=%d\n", Gain);
        return BADRANGE;
    }
}
int mcE_1608::cbAIn(int Chan, int Gain, USHORT *DataValue)
{
    uint16_t value;
    int range = gainToRange(Gain);
    if (range == BADRANGE) {
        printf("Unsupported Gain=%d\n", Gain);
        return BADRANGE;
    }
    if (!AIn_E1608(&deviceInfo_, Chan, (uint8_t)range, &value)) {
       return BADBOARD;
    }
    *DataValue = value;
    return NOERRORS;
}

int mcE_1608::cbAInScan(int LowChan, int HighChan, long Count, long *Rate, 
                      int Gain, HGLOBAL MemHandle, int Options)
{
    uint8_t options = 0x0;
    // We assume that aLoadQueue has previously been called and deviceInfo_.queue[0] contains number of channels
    // being scanned.
    aiScanNumChans_ = deviceInfo_.queue[0];
    double frequency = *Rate * aiScanNumChans_;
    aiScanTotalElements_ = Count;
    if (Options & SCALEDATA) {
        aiScanScaledBuffer_ = (double *)MemHandle;
        aiScanIsScaled_ = true;
    } else {
        aiScanUnscaledBuffer_ = (uint16_t *)MemHandle;
        aiScanIsScaled_ = false;
    }
    uint32_t scanPoints = Count / aiScanNumChans_;
printf("mcE_1608::aInScan Count=%ld, frequency=%f, Options=0x%x\n", Count, frequency, Options);
    if (!AInScanStart_E1608(&deviceInfo_, scanPoints, frequency, options)) {
        printf("Error calling AInStartScan_E1608\n");
        return BADBOARD;
    }
    aiScanAcquiring_ = true;
    return NOERRORS;
}

int mcE_1608::cbALoadQueue(short *ChanArray, short *GainArray, int NumChans)
{
    deviceInfo_.queue[0] = NumChans;
    int range;
    for (int i=0; i<NumChans; i++) {
        deviceInfo_.queue[2*i+1] = ChanArray[i];
        range = gainToRange(GainArray[i]);
        if (range == BADRANGE) {
            printf("Unsupported Gain=%d\n", GainArray[i]);
            return BADRANGE;
        }
        deviceInfo_.queue[2*i+2] = (uint8_t)range;    
    }
    if (!AInQueueW_E1608(&deviceInfo_)) {
        printf("Error calling AInQueueW_E1608\n");
        return BADBOARD;
    }
    return NOERRORS;
}

int mcE_1608::cbAOut(int Chan, int Gain, USHORT DataValue)
{
      if (!AOut_E1608(&deviceInfo_, Chan, DataValue)) {
         return BADBOARD;
      }
      return NOERRORS;
}

int mcE_1608::cbCIn32(int CounterNum, ULONG *Count)
{
    uint32_t counts;
      if (!CounterR_E1608(&deviceInfo_, &counts)) {
         return BADBOARD;
      }
      *Count = counts;
      return NOERRORS;
}

int mcE_1608::cbCLoad32(int RegNum, ULONG LoadValue)
{
      if (!ResetCounter_E1608(&deviceInfo_)) {
         return BADBOARD;
      }
      return NOERRORS;
}

int mcE_1608::cbDBitOut(int PortType, int BitNum, USHORT BitValue)
{
    uint8_t value;
    uint8_t mask = 0x1 << BitNum;
    // Read the current output register
    if (!DOutR_E1608(&deviceInfo_, &value)) {
        return BADBOARD;
    }
    // Set or clear the bit
    if (BitValue) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    if (!DOut_E1608(&deviceInfo_, value)) {
        return BADBOARD;
    }
    return NOERRORS;
}

int mcE_1608::cbDConfigBit(int PortType, int BitNum, int Direction)
{
    uint8_t value;
    uint8_t mask = 0x1 << BitNum;
    // Read the current output register
    if (!DConfigR_E1608(&deviceInfo_, &value)) {
        return BADBOARD;
    }
    // Set or clear the bit
    if (Direction == DIGITALIN) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    if (!DConfigW_E1608(&deviceInfo_, value)) {
        return BADBOARD;
    }
    return NOERRORS;
}

int mcE_1608::cbDIn(int PortType, USHORT *DataValue)
{
    uint8_t value;
    if (!DIn_E1608(&deviceInfo_, &value)) {
        return BADBOARD;
    }
    *DataValue = value;
    return NOERRORS;
}
