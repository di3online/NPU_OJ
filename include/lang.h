#ifndef _LANG_INCLUDED_
#define _LANG_INCLUDED_

#include "common.h"
#include "dataitem.h"

class LangJudge {
public:
    virtual Noj_Judge_Mode get_mode() const = 0;
    virtual Noj_Language get_lang() const = 0;
    virtual Noj_State judge(Judge *jdg, Submission &submit) = 0;
    bool match(Submission &submit);
    virtual ~LangJudge() { }
};

bool operator==(const LangJudge &a, const LangJudge &b);

class LangC : public LangJudge {
protected:
    virtual Result compile(Submission submit, const ResourceLimit &rl) = 0;
    Result launch(Submission submit, ResourceLimit &rl);
    Result check(Judge *jdg, Submission submit, TestCase tc);
public:
    LangC() {}
    Noj_State judge(Judge *jdg, Submission &submit); 
    virtual Noj_Judge_Mode get_mode() const = 0;
    virtual Noj_Language get_lang() const = 0;
};

class LangCDebug : public LangC {
protected:
    char *compile_script_path;
    Result compile(Submission submit, const ResourceLimit &rl);
    virtual const char *get_script_path();
public:
    LangCDebug();
    Noj_State judge(Judge *jdg, Submission &submit); 
    Noj_Judge_Mode get_mode() const;
    Noj_Language get_lang() const;
}; 

class LangCRelease : public LangCDebug {
protected:
public:
    LangCRelease();
    Noj_State judge(Judge *jdg, Submission &submit); 
    Noj_Judge_Mode get_mode() const;
    Noj_Language get_lang() const;
};


class LangCppDebug : public LangCDebug {
protected:
public:
    LangCppDebug();
    Noj_State judge(Judge *jdg, Submission &submit); 
    Noj_Judge_Mode get_mode() const;
    Noj_Language get_lang() const;
}; 

class LangCppRelease : public LangCDebug {
protected:
public:
    LangCppRelease();
    Noj_State judge(Judge *jdg, Submission &submit); 
    Noj_Judge_Mode get_mode() const;
    Noj_Language get_lang() const;
};
#endif
