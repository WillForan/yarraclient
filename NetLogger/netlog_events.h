#ifndef NETLOG_EVENTS_H
#define NETLOG_EVENTS_H
namespace EventInfo {
    enum class Type : int {
        Generic,
        Boot,
        Shutdown,
        Upload
    };

    enum class Detail : int {
        Generic,
        Success,
        Failure,
        LowDiskSpace
    };

    enum class SourceType : int {
        RDS=0,
        SAC,
        YarraServer
    };

    enum class Severity : int {
        Routine=0,
        Warning,
        Error,
        FatalError
    };
}
#endif // NETLOG_EVENTS_H
