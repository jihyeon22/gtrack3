#ifndef _XML_PROTOCOL_H_
#define _XML_PROTOCOL_H_

#include "standard_protocol.h"

#define LEN_XML_START			6
#define LEN_XML_END				7

#define LEN_XML_HEADER_START	4
#define LEN_XML_HEADER_END		5

#define LEN_XML_BODY_START		4
#define LEN_XML_BODY_END		5

#define LEN_XML_RECORD_START	3
#define LEN_XML_RECORD_END		5

#define LEN_XML_ITEM_START		4
#define LEN_XML_ITEM_END		6

#pragma pack(push, 1)

struct tacom_xml_hdr {
	char xml_starter[LEN_XML_START];	//<xml>\n
	char xml_header_starter[LEN_XML_HEADER_START];	//<H>\n
	
	char xml_item_starter1[LEN_XML_ITEM_START];	//<I1>
	char vehicle_model[20];
	char xml_item_end1[LEN_XML_ITEM_END];	//</I1>\n

	char xml_item_starter2[LEN_XML_ITEM_START];
	char vehicle_id_num[17];
	char xml_item_end2[LEN_XML_ITEM_END];
	
	char xml_item_starter3[LEN_XML_ITEM_START];
	char vehicle_type[2];
	char xml_item_end3[LEN_XML_ITEM_END];
	
	char xml_item_starter4[LEN_XML_ITEM_START];
	char registration_num[12];
	char xml_item_end4[LEN_XML_ITEM_END];
	
	char xml_item_starter5[LEN_XML_ITEM_START];
	char business_license_num[10];
	char xml_item_end5[LEN_XML_ITEM_END];
	
	char xml_item_starter6[LEN_XML_ITEM_START];	
	char driver_code[18];						
	char xml_item_end6[LEN_XML_ITEM_END];
	
	char xml_header_ender[LEN_XML_HEADER_END];	//</H>\n
	char xml_body_starter[LEN_XML_BODY_START];	//<B>\n
}__attribute__((packed));
typedef struct tacom_xml_hdr tacom_xml_hdr_t;

struct tacom_xml_data {
	char xml_record_starter[LEN_XML_RECORD_START];	//<R>

	tacom_std_data_t std_body;

	char xml_record_ender[LEN_XML_RECORD_END]; //</R>\n
}__attribute__((packed));
typedef struct tacom_xml_data tacom_xml_data_t;

struct tacom_xml_end {
	char xml_body_ender[LEN_XML_BODY_END];
	char xml_ender[LEN_XML_END];
}__attribute__((packed));
typedef struct tacom_xml_end tacom_xml_end_t;
#pragma pack(pop)

int xml_wrapping(char* src_buf, char *strm, int r_num);

#endif 
