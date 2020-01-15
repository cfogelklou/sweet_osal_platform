/**
* COPYRIGHT	(c)	Applicaudia 2020
* @file     enum_table_descriptions.h
* @brief    This macro provides for a table of enumerations
*           and associated strings.
*           This part defines the descriptions.
*/
#ifndef ENUM_TABLE_DESCRIPTIONS_H__
#define ENUM_TABLE_DESCRIPTIONS_H__

#undef ENUM_MACRO
#define ENUM_MACRO(name, description) description,

#undef ENUM_MACRO_LAST
#define ENUM_MACRO_LAST(name, description) description

#endif

