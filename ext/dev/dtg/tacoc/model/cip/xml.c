#include <stdio.h>
#include "xml_protocol.h"
#include "standard_protocol.h"

void make_xml_header(tacom_xml_hdr_t *hdr)
{
	memcpy(hdr->xml_starter, 			"<xml>\n", 	LEN_XML_START);
	memcpy(hdr->xml_header_starter, 	"<H>\n", 	LEN_XML_HEADER_START);

	memcpy(hdr->xml_item_starter1, 		"<I1>", 	LEN_XML_ITEM_START);
	memcpy(hdr->xml_item_end1, 			"</I1>\n",	LEN_XML_ITEM_END);
	memcpy(hdr->xml_item_starter2, 		"<I2>", 	LEN_XML_ITEM_START);
	memcpy(hdr->xml_item_end2, 			"</I2>\n", 	LEN_XML_ITEM_END);
	memcpy(hdr->xml_item_starter3, 		"<I3>", 	LEN_XML_ITEM_START);
	memcpy(hdr->xml_item_end3, 			"</I3>\n", 	LEN_XML_ITEM_END);
	memcpy(hdr->xml_item_starter4, 		"<I4>", 	LEN_XML_ITEM_START);
	memcpy(hdr->xml_item_end4, 			"</I4>\n", 	LEN_XML_ITEM_END);
	memcpy(hdr->xml_item_starter5,		"<I5>", 	LEN_XML_ITEM_START);
	memcpy(hdr->xml_item_end5,		 	"</I5>\n", 	LEN_XML_ITEM_END);
	memcpy(hdr->xml_item_starter6, 		"<I6>", 	LEN_XML_ITEM_START);
	memcpy(hdr->xml_item_end6, 			"</I6>\n", 	LEN_XML_ITEM_END);
	
	memcpy(hdr->xml_header_ender, 		"</H>\n", 	LEN_XML_HEADER_END);
	memcpy(hdr->xml_body_starter,		"<B>\n", 	LEN_XML_BODY_START);

}

void make_xml_record(tacom_xml_data_t *data)
{
	memcpy(data->xml_record_starter,	"<R>",		LEN_XML_RECORD_START);
	memcpy(data->xml_record_ender,		"</R>\n",	LEN_XML_RECORD_END);
}

void make_xml_end(tacom_xml_end_t *end)
{
	memcpy(end->xml_body_ender, 		"</B>\n", 	LEN_XML_BODY_END);
	memcpy(end->xml_ender, 				"</xml>\n", LEN_XML_END);
}

int xml_wrapping(char* src_buf, char *dst_buf, int r_num)
{
	int src_idx = 0;
	int dst_idx = 0;
	tacom_xml_hdr_t *xml_hdr;
	tacom_xml_data_t *xml_data;
	tacom_xml_end_t *xml_end;

	tacom_std_hdr_t *std_hdr;
	tacom_std_data_t *std_data;

	std_hdr = (tacom_std_hdr_t *) &src_buf[src_idx];
	xml_hdr = (tacom_xml_hdr_t *) &dst_buf[dst_idx];

	memcpy(xml_hdr->vehicle_model, std_hdr->vehicle_model, 20);
	memcpy(xml_hdr->vehicle_id_num, std_hdr->vehicle_id_num, 17);
	memcpy(xml_hdr->vehicle_type, std_hdr->vehicle_type, 2);
	memcpy(xml_hdr->registration_num, std_hdr->registration_num, 12);
	memcpy(xml_hdr->business_license_num, std_hdr->business_license_num, 10);
	memcpy(xml_hdr->driver_code, std_hdr->driver_code, 18);

	make_xml_header(xml_hdr);
	src_idx += sizeof(tacom_std_hdr_t);
	dst_idx += sizeof(tacom_xml_hdr_t);
	r_num -= sizeof(tacom_std_hdr_t);

	while (r_num > 0) {
		std_data = (tacom_std_data_t *) &src_buf[src_idx];
		xml_data = (tacom_xml_data_t *) &dst_buf[dst_idx];
		memcpy(&(xml_data->std_body), std_data, sizeof(tacom_std_data_t));
		make_xml_record(xml_data);
		src_idx += sizeof(tacom_std_data_t);
		dst_idx += sizeof(tacom_xml_data_t);
		r_num -= sizeof(tacom_std_data_t);
	}

	xml_end = (tacom_xml_data_t *) &dst_buf[dst_idx];
	make_xml_end(xml_end);
	dst_idx += sizeof(tacom_xml_end_t);
	return dst_idx;
}

