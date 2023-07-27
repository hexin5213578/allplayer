// INIConfig.h: interface for the as_ini_config class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INICONFIG_H__C94A8432_F6BA_4C88_99BA_7C5CA0139EC7__INCLUDED_)
#define AFX_INICONFIG_H__C94A8432_F6BA_4C88_99BA_7C5CA0139EC7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
#include <string>
#include <fstream>
#include <map>

using namespace std;



//key = value,key=value .........
typedef map<string , string> items; 
//[section] ---> key key key .........
typedef map<string , items> sections;


enum ErrorType
{
    INI_SUCCESS         =  0,
    INI_OPENFILE_FAIL   = -1,
    INI_SAVEFILE_FAIL   = -2,
    INI_ERROR_NOSECTION = -3,
    INI_ERROR_NOKEY     = -4
};

#define     INI_CONFIG_MAX_SECTION_LEN        63

#define     INI_CONFIG_LINE_SIZE_MAX          2048

class as_ini_config  
{
public:
    as_ini_config();
    virtual ~as_ini_config();
public:
    virtual int32_t ReadIniFile(const string &filename);
    int32_t SaveIniFile(const string & filename = "");
    void SetValue(const string & section, const string & key, 
        const string & value);
    virtual int32_t GetValue(const string & section, const string & key, 
        string & value);

    int32_t GetSection(const string & section, items & keyValueMap);
    int32_t ExportToFile(const string &filename);

    int32_t ExportToFileExceptPointed(const string &filename, 
                            uint32_t ulSectionCont, const char szSection[][INI_CONFIG_MAX_SECTION_LEN+1]);
                            
private:

     string m_strFilePath;    
     sections m_SectionMap;

};

#endif // !defined(AFX_INICONFIG_H__C94A8432_F6BA_4C88_99BA_7C5CA0139EC7__INCLUDED_)
