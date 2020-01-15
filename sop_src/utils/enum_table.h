/**
  * COPYRIGHT	(c)	Applicaudia 2020
  * @file     enum_table_defs
  * @brief    This macro provides for a table of enumerations
  *           and associated strings.
  *           This part defines the enum, used to reference the strings.
  */
#ifndef ENUM_TABLE_H__
#define ENUM_TABLE_H__

/** 
  1.) You need to define an enum type to be used to address the table entries

  #include "enum_table.h"

  typedef enum MsgETag {
    #include "my_enums.inc"
  } MsgE;

  2.) Declare your string tables (one for enum names, the other for associated descriptions)

  extern const char * MsgENames[];
  extern const char * MsgEDesciptions[];
  
  3.) Then, define the "my_enums.inc" file which defines all enums

  // my_enums.inc

  #ifndef ENUM_MACRO
  #define ENUM_MACRO(name, description)
  #endif

  ENUM_MACRO( MSG_ERROR, "error!" )
  ENUM_MACRO( MSG_WARNING, "warning!" )
  ENUM_MACRO( MSG_OK, "ok!" )
  ...
  ENUM_MACRO_LAST(MSG_LAST, NULL )


  4.) Create a source file to hold your tables

  // Declare printable enum names
  #include "enum_table_names.h"
  const char * MsgENames = {
  #include "my_enums.inc"
  };

  // Declare printable enum description table
  #include "enum_table_descriptions.h"
  const char * MsgEDescriptions = {
  #include "my_enums.inc"
  };

*/


#undef ENUM_MACRO
#define ENUM_MACRO(name, desc) name,

#undef ENUM_MACRO_LAST
#define ENUM_MACRO_LAST(name, desc) name

#endif
