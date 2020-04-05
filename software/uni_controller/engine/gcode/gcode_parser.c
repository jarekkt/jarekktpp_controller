/*!
    \file gcode_parser.c

    \brief
*/


#include "system.h"
#include "services.h"
#include "middleware.h"
#include "engine.h"
#include "gcode_parser.h"


typedef struct
{
	char 		fn_letter;
	uint32_t 	fn_number;
	gcode_fn_e  fn_id;

	struct
	{
		char 				token_letter;
		gcode_item_e		token_id;
		gcode_value_type_e	token_val_type;
	}tokens[GCODE_I_CNT];

}gcode_parse_table_t;



gcode_parse_table_t  parse_table[] =
{
		{
			'G',0, GCODE_F_G0,
			{
				{ 'X' , GCODE_I_X, GCODE_V_FLOAT} ,
				{ 'Y' , GCODE_I_Y, GCODE_V_FLOAT} ,
				{ 'Z' , GCODE_I_Z, GCODE_V_FLOAT} ,
				{ 'F' , GCODE_I_F, GCODE_V_FLOAT} ,
			}
		},
		{
			'G',1, GCODE_F_G1,
			{
				{ 'X' , GCODE_I_X, GCODE_V_FLOAT} ,
				{ 'Y' , GCODE_I_Y, GCODE_V_FLOAT} ,
				{ 'Z' , GCODE_I_Z, GCODE_V_FLOAT} ,
				{ 'F' , GCODE_I_F, GCODE_V_FLOAT} ,
			}
		},

};

void gcode_parser_init()
{

}

void     gcode_parser_error(char * buffer,uint32_t buffer_cnt)
{

}

int32_t  gcode_parser_execute(gcode_command_t * cmd,const char * cmd_line)
{
	int result = -1;


    char *string,*found;

    string = strdup("Hello there, peasants!");
    printf("Original string: '%s'\n",string);

    while( (found = strsep(&string," ")) != NULL )
        printf("%s\n",found);

	return result;
}
