#ifndef YCT_TWIX_HEADER_H
#define YCT_TWIX_HEADER_H


#ifdef _MSC_VER
#  include <cstdint>
#else
#  include <inttypes.h>
#  include <cstdlib>
#endif


#if defined(__GNUC__)
#    define PACKED_MEMBER( __type__, __name__ ) __type__ __name__ __attribute__ ((packed))
#    define PACKED_STRUCT( __type__, __name__ ) __type__ __name__
#else
#    pragma pack(push, pack_header_included)
#    pragma pack(1)
#    define PACKED_MEMBER( __type__, __name__ ) __type__ __name__
#    define PACKED_STRUCT( __type__, __name__ ) __type__ __name__
#endif


enum EIM_BIT
{
    ACQEND = 0,
    RTFEEDBACK,
    HPFEEDBACK,
    ONLINE,
    OFFLINE,
    SYNCDATA,
    LASTSCANINCONCAT = 8,
    RAWDATACORRECTION = 10,
    LASTSCANINMEAS,
    SCANSCALEFACTOR,
    SECONDHADAMARPULSE,
    REFPHASESTABSCAN,
    PHASESTABSCAN,
    D3FFT,
    SIGNREV,
    PHASEFFT,
    SWAPPED,
    POSTSHAREDLINE,
    PHASCOR,
    PATREFSCAN,
    PATREFANDIMASCAN,
    REFLECT,
    NOISEADJSCAN,
    SHARENOW,
    LASTMEASUREDLINE,
    FIRSTSCANINSLICE,
    LASTSCANINSLICE,
    TREFFECTIVEBEGIN,
    TREFFECTIVEEND
};


// VB Header
namespace VB
{
    typedef struct MeasHeader
    {
        PACKED_MEMBER(uint32_t, ulDMALength);
        PACKED_MEMBER( int32_t, lMeasUID);
        PACKED_MEMBER(uint32_t, ulScanCounter);
        PACKED_MEMBER(uint32_t, ulTimeStamp);
        PACKED_MEMBER(uint32_t, ulPMUTimeStamp);
        PACKED_MEMBER(uint32_t, aulEvalInfoMask[2]);
        PACKED_MEMBER(uint16_t, ushSamplesInScan);
        PACKED_MEMBER(uint16_t, ushUsedChannels);
        PACKED_MEMBER(uint16_t, sLC[14]);
        PACKED_STRUCT(uint16_t, sCutOff[2]);
        PACKED_MEMBER(uint16_t, ushKSpaceCentreColumn);
        PACKED_MEMBER(uint16_t, ushCoilSelect);
        PACKED_MEMBER(   float, fReadOutOffcentre);
        PACKED_MEMBER(uint32_t, ulTimeSinceLastRF);
        PACKED_MEMBER(uint16_t, ushKSpaceCentreLineNo);
        PACKED_MEMBER(uint16_t, ushKSpaceCentrePartitionNo);
        PACKED_MEMBER(uint16_t, aushIceProgramPara[4]);
        PACKED_MEMBER(uint16_t, aushFreePara[4]);
        PACKED_STRUCT(  float, sSD[7]);
        PACKED_MEMBER(uint16_t, ushChannelId);
        PACKED_MEMBER(uint16_t, ushPTABPosNeg);
    } MeasHeader;
    const size_t MEAS_HEADER_LEN = sizeof(MeasHeader);
}

// VD Header
namespace VD
{
    typedef struct MeasHeader
    {
        PACKED_MEMBER(uint32_t, ulFlagsAndDMALength);
        PACKED_MEMBER( int32_t, lMeasUID);
        PACKED_MEMBER(uint32_t, ulScanCounter);
        PACKED_MEMBER(uint32_t, ulTimeStamp);
        PACKED_MEMBER(uint32_t, ulPMUTimeStamp);
        PACKED_MEMBER(uint16_t, ushSystemType);
        PACKED_MEMBER(uint16_t, ulPTABPosDelay);
        PACKED_MEMBER( int32_t,	lPTABPosX);
        PACKED_MEMBER( int32_t,	lPTABPosY);
        PACKED_MEMBER( int32_t,	lPTABPosZ);
        PACKED_MEMBER(uint32_t,	ulReserved1);
        PACKED_MEMBER(uint32_t, aulEvalInfoMask[2]);
        PACKED_MEMBER(uint16_t, ushSamplesInScan);
        PACKED_MEMBER(uint16_t, ushUsedChannels);
        PACKED_MEMBER(uint16_t, sLC[14]);
        PACKED_STRUCT(uint16_t, sCutOff[2]);
        PACKED_MEMBER(uint16_t, ushKSpaceCentreColumn);
        PACKED_MEMBER(uint16_t, ushCoilSelect);
        PACKED_MEMBER(   float, fReadOutOffcentre);
        PACKED_MEMBER(uint32_t, ulTimeSinceLastRF);
        PACKED_MEMBER(uint16_t, ushKSpaceCentreLineNo);
        PACKED_MEMBER(uint16_t, ushKSpaceCentrePartitionNo);
        PACKED_STRUCT(   float, sSD[7]);
        PACKED_MEMBER(uint16_t, aushIceProgramPara[24]);
        PACKED_MEMBER(uint16_t, aushReservedPara[4]);
        PACKED_MEMBER(uint16_t, ushApplicationCounter);
        PACKED_MEMBER(uint16_t, ushApplicationMask);
        PACKED_MEMBER(uint32_t, ulCRC);
    } MeasHeader;
    const size_t MEAS_HEADER_LEN = sizeof(MeasHeader);

    // Channel header
    typedef struct ChannelHeader
    {
        PACKED_MEMBER(uint32_t, ulTypeAndChannelLength);
        PACKED_MEMBER( int32_t, lMeasUID);
        PACKED_MEMBER(uint32_t, ulScanCounter);
        PACKED_MEMBER(uint32_t, ulReserved1);
        PACKED_MEMBER(uint32_t, ulSequenceTime);
        PACKED_MEMBER(uint32_t, ulUnused2);
        PACKED_MEMBER(uint16_t, ulChannelId);
        PACKED_MEMBER(uint16_t, ulUnused3);
        PACKED_MEMBER(uint32_t, ulCRC);
    } ChannelHeader;
    const size_t CHANNEL_HEADER_LEN = sizeof(ChannelHeader);

    // Entry header
    typedef struct EntryHeader
    {
        PACKED_MEMBER(uint32_t, MeasID);
        PACKED_MEMBER(uint32_t, FieldID);
        PACKED_MEMBER(uint64_t, MeasOffset);
        PACKED_MEMBER(uint64_t, MeasLen);
        char PatientName [64];
        char ProtocolName[64];
    } EntryHeader;
    const size_t ENTRY_HEADER_LEN = sizeof(EntryHeader);
}

// Sync header length
const size_t SYNC_HEADER_SIZE = 64;


#endif // YCT_TWIX_HEADER_H
