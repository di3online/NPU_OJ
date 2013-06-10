/******************************************
 * 
 * Author:  Laishzh
 * File:    common.h
 * Email:   laishengzhang(at)gmail.com 
 * 
 *******************************************/

#ifndef NOJ_COMMON_INCLUDE
#define NOJ_COMMON_INCLUDE

const long BUFFER_SIZE = 256;

enum _Noj_Mode {

};

enum Noj_Judge_Mode {
    NojMode_UnSet = 0x00,
    NojMode_Release = 0x01,
    NojMode_Debug = 0x02
};

enum Noj_Language {
    NojLang_Unkown = 0,
    NojLang_C,
    NojLang_CPP,
    NojLang_Java,
    NojLang_Pascal
};

enum Noj_Result {
    NojRes_SystemError = 0,
    NojRes_Init,
    NojRes_Pending,
    NojRes_Judging,
    NojRes_NoCompile,
    NojRes_CompileError,
    NojRes_CompilePass,
    NojRes_RuntimeError,
    NojRes_TimeLimitExceed,
    NojRes_MemoryLimitExceed,
    NojRes_OutputLimitExceed,
    NojRes_IllegalSystemCall,
    NojRes_SegmentFault,
    NojRes_BusError,
    NojRes_Abort,
    NojRes_RunPass,
    NojRes_WrongAnswer,
    NojRes_PresentationError,
    NojRes_CorrectAnswer,
    NojRes_Accept,
    NojRes_NotAccept
};

enum Noj_State {
    Noj_ServerError,
    Noj_DBError,
    Noj_SQLQueryFail,
    Noj_SQLQueryNothing,
    Noj_FileNotDownload,
    Noj_PermissionError,
    Noj_Normal
};

#endif
