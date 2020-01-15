/**
  * COPYRIGHT	(c)	Applicaudia 2020
  * @file     enum_table_names.h
  * @brief    This macro provides for a table of enumerations
  *           and associated strings.
  *           This part defines the names.
  */
#ifndef ENUM_TABLE_NAMES_H__
#define ENUM_TABLE_NAMES_H__

#undef ENUM_MACRO
#define ENUM_MACRO(name, description) #name,

#undef ENUM_MACRO_LAST
#define ENUM_MACRO_LAST(name, description) #name

#endif
