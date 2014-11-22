#ifndef _PARSE_XML_H
#define _PARSE_XML_H

#include <libxml/parser.h>
#include <libxml/xmlIO.h>
#include <libxml/xinclude.h>

typedef struct bit_s {
	int bitrate;
	struct bit_s* next;
}bit_t;

bit_t* parse_xml(char* filename);
int parseMedia(xmlDocPtr doc, xmlNodePtr cur);

#endif