/*****************************************************************************
 * dbf.c
 *****************************************************************************
 * Library to read information from dBASE files
 * Author: Bjoern Berg, clergyman@gmx.de
 * (C) Copyright 2004, Björn Berg
 *
 *****************************************************************************
 * Permission to use, copy, modify and distribute this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation. The
 * author makes no representations about the suitability of this software for
 * any purpose. It is provided "as is" without express or implied warranty.
 *
 * $Id: dbf.c,v 1.8 2004/09/07 16:08:23 steinm Exp $
 ****************************************************************************/


#include <time.h>
#include "../include/libdbf/libdbf.h"
#include "dbf.h"

/* get_db_version() {{{
 * Convert version field of header into human readable string.
 */
const char *get_db_version(int version) {
	static char name[31];

	switch (version) {
		case 0x02:
			// without memo fields
			return "FoxBase";
		case 0x03:
			// without memo fields
			return "FoxBase+/dBASE III+";
		case 0x04:
			// without memo fields
			return "dBASE IV";
		case 0x05:
			// without memo fields
			return "dBASE 5.0";
		case 0x83:
			return "FoxBase+/dBASE III+";
		case 0x8B:
			return "dBASE IV";
		case 0x30:
			// without memo fields
			return "Visual FoxPro";
		case 0xF5:
			// with memo fields
			return "FoxPro 2.0";
		default:
			sprintf(name, _("Unknown (code 0x%.2X)"), version);
			return name;
	}
}
/* }}} */

/* static dbf_ReadHeaderInfo() {{{
 * Reads header from file into struct
 */
static int dbf_ReadHeaderInfo(P_DBF *p_dbf)
{
	DB_HEADER *header;
	if(NULL == (header = malloc(sizeof(DB_HEADER)))) {
		return -1;
	}
	if ((read( p_dbf->dbf_fh, header, sizeof(DB_HEADER))) == -1 ) {
		return -1;
	}

	/* Endian Swapping */
	header->header_length = rotate2b(header->header_length);
	header->record_length = rotate2b(header->record_length);
	header->records = rotate4b(header->records);
	p_dbf->header = header;

	return 0;
}
/* }}} */

/* static dbf_WriteHeaderInfo() {{{
 * Write header into file
 */
static int dbf_WriteHeaderInfo(P_DBF *p_dbf, DB_HEADER *header)
{
	time_t ps_calendar_time;
	struct tm *ps_local_tm;

	DB_HEADER *newheader = malloc(sizeof(DB_HEADER));
	if(NULL == newheader) {
		return -1;
	}
	memcpy(newheader, header, sizeof(DB_HEADER));

	ps_calendar_time = time(NULL);
	if(ps_calendar_time != (time_t)(-1)) {
		ps_local_tm = localtime(&ps_calendar_time);
		newheader->last_update[0] = ps_local_tm->tm_year;
		newheader->last_update[1] = ps_local_tm->tm_mon+1;
		newheader->last_update[2] = ps_local_tm->tm_mday;
	}

	newheader->header_length = rotate2b(newheader->header_length);
	newheader->record_length = rotate2b(newheader->record_length);
	newheader->records = rotate4b(newheader->records);

	/* Make sure the header is written at the beginning of the file
	 * because this function is also called after each record has
	 * been written.
	 */
	lseek(p_dbf->dbf_fh, 0, SEEK_SET);
	if ((write( p_dbf->dbf_fh, newheader, sizeof(DB_HEADER))) == -1 ) {
		free(newheader);
		return -1;
	}
	free(newheader);
	return 0;
}
/* }}} */

/* static dbf_ReadFieldInfo() {{{
 * Sets p_dbf->fields to an array of DB_FIELD containing the specification
 * for all columns.
 */
static int dbf_ReadFieldInfo(P_DBF *p_dbf)
{
	int columns, i, offset;
	DB_FIELD *fields;

	columns = dbf_NumCols(p_dbf);

	if(NULL == (fields = malloc(columns * sizeof(DB_FIELD)))) {
		return -1;
	}

	lseek(p_dbf->dbf_fh, sizeof(DB_HEADER), SEEK_SET);

	if ((read( p_dbf->dbf_fh, fields, columns * sizeof(DB_FIELD))) == -1 ) {
		perror(_("In function dbf_ReadFieldInfo(): "));
		return -1;
	}
	p_dbf->fields = fields;
	p_dbf->columns = columns;
	/* The first byte of a record indicates whether it is deleted or not. */
	offset = 1;
	for(i = 0; i < columns; i++) {
		fields[i].field_offset = offset;
		offset += fields[i].field_length;
	}

	return 0;
}
/* }}} */

/* static dbf_WriteFieldInfo() {{{
 * Writes the field specification into the output file
 */
static int dbf_WriteFieldInfo(P_DBF *p_dbf, DB_FIELD *fields, int numfields)
{
	lseek(p_dbf->dbf_fh, sizeof(DB_HEADER), SEEK_SET);

	if ((write( p_dbf->dbf_fh, fields, numfields * sizeof(DB_FIELD))) == -1 ) {
		perror(_("In function dbf_WriteFieldInfo(): "));
		return -1;
	}

	write(p_dbf->dbf_fh, "\r\0", 2);

	return 0;
}
/* }}} */

/* dbf_Open() {{{
 * Open the a dbf file and returns file handler
 */
P_DBF *dbf_Open(const char *file)
{
	P_DBF *p_dbf;
	if(NULL == (p_dbf = malloc(sizeof(P_DBF)))) {
		return NULL;
	}

	if (file[0] == '-' && file[1] == '\0') {
		p_dbf->dbf_fh = fileno(stdin);
	} else if ((p_dbf->dbf_fh = open(file, O_RDONLY|O_BINARY)) == -1) {
		free(p_dbf);
		return NULL;
	}

	p_dbf->header = NULL;
	if(0 > dbf_ReadHeaderInfo(p_dbf)) {
		free(p_dbf);
		return NULL;
	}

	p_dbf->fields = NULL;
	if(0 > dbf_ReadFieldInfo(p_dbf)) {
		free(p_dbf->header);
		free(p_dbf);
		return NULL;
	}

	p_dbf->cur_record = 0;

	return p_dbf;
}
/* }}} */

/* dbf_CreateFH() {{{
 * Create a new dbf file and returns file handler
 */
P_DBF *dbf_CreateFH(int fh, DB_FIELD *fields, int numfields)
{
	P_DBF *p_dbf;
	DB_HEADER *header;
	int reclen, i;

	if(NULL == (p_dbf = malloc(sizeof(P_DBF)))) {
		return NULL;
	}

	p_dbf->dbf_fh = fh;

	if(NULL == (header = malloc(sizeof(DB_HEADER)))) {
		return NULL;
	}
	reclen = 0;
	for(i=0; i<numfields; i++) {
		reclen += fields[i].field_length;
	}
	memset(header, 0, sizeof(DB_HEADER));
	header->version = FoxBasePlus;
	/* Add 1 to record length for deletion flog */
	header->record_length = reclen+1;
	header->header_length = sizeof(DB_HEADER) + numfields * sizeof(DB_FIELD) + 2;
	if(0 > dbf_WriteHeaderInfo(p_dbf, header)) {
		free(p_dbf);
		return NULL;
	}
	p_dbf->header = header;

	if(0 > dbf_WriteFieldInfo(p_dbf, fields, numfields)) {
		free(p_dbf->header);
		free(p_dbf);
		return NULL;
	}
	p_dbf->fields = fields;

	p_dbf->cur_record = 0;

	return p_dbf;
}
/* }}} */

/* dbf_Create() {{{
 * Create a new dbf file and returns file handler
 */
P_DBF *dbf_Create(const char *file, DB_FIELD *fields, int numfields)
{
	int fh;

	if (file[0] == '-' && file[1] == '\0') {
		fh = fileno(stdout);
	} else if ((fh = open(file, O_WRONLY|O_BINARY)) == -1) {
		return NULL;
	}

	return(dbf_CreateFH(fh, fields, numfields));
}
/* }}} */

/* dbf_Close() {{{
 * Close the current open dbf file and free all memory
 */
int dbf_Close(P_DBF *p_dbf)
{
	if(p_dbf->header)
		free(p_dbf->header);

	if(p_dbf->fields)
		free(p_dbf->fields);

	if ( p_dbf->dbf_fh == fileno(stdin) )
		return 0;

	if( (close(p_dbf->dbf_fh)) == -1 ) {
		return -1;
	}

	free(p_dbf);

	return 0;
}
/* }}} */

/******************************************************************************
	Block with functions to get information about the amount of
		- rows and
		- columns
 ******************************************************************************/

/* dbf_NumRows() {{{
 * Returns the number of records.
 */
int dbf_NumRows(P_DBF *p_dbf)
{
	if ( p_dbf->header->records > 0 ) {
		return p_dbf->header->records;
	} else {
		perror(_("In function dbf_NumRows(): "));
		return -1;
	}

	return 0;
}
/* }}} */

/* dbf_NumCols() {{{
 * Returns the number of fields.
 */
int dbf_NumCols(P_DBF *p_dbf)
{
	if ( p_dbf->header->header_length > 0) {
		// TODO: Backlink muss noch eingerechnet werden
		return ((p_dbf->header->header_length - sizeof(DB_HEADER) -1)
					 / sizeof(DB_FIELD));
	} else {
		perror(_("In function dbf_NumCols(): "));
		return -1;
	}

	return 0;
}
/* }}} */

/******************************************************************************
	Block with functions to get/set information about the columns
 ******************************************************************************/

/* dbf_ColumnName() {{{
 * Returns the name of a column. Column names cannot be longer than
 * 11 characters.
 */
const char *dbf_ColumnName(P_DBF *p_dbf, int column)
{
	if ( column >= p_dbf->columns ) {
		return "invalid";
	}

	return p_dbf->fields[column].field_name;
}
/* }}} */

/* dbf_ColumnSize() {{{
 */
int dbf_ColumnSize(P_DBF *p_dbf, int column)
{
	if ( column >= p_dbf->columns ) {
		return -1;
	}

	return (int) p_dbf->fields[column].field_length;
}
/* }}} */

/* dbf_ColumnType() {{{
 */
const char dbf_ColumnType(P_DBF *p_dbf, int column)
{
	if ( column >= p_dbf->columns ) {
		return -1;
	}

	return p_dbf->fields[column].field_type;
}
/* }}} */

/* dbf_ColumnDecimals() {{{
 */
int dbf_ColumnDecimals(P_DBF *p_dbf, int column)
{
	if ( column >= p_dbf->columns ) {
		return -1;
	}

	return p_dbf->fields[column].field_decimals;
}
/* }}} */

/* dbf_ColumnAddress() {{{
 */
u_int32_t dbf_ColumnAddress(P_DBF *p_dbf, int column)
{
	if ( column >= p_dbf->columns ) {
		return -1;
	}

	return p_dbf->fields[column].field_address;
}
/* }}} */

/* dbf_SetField() {{{
 */
int dbf_SetField(DB_FIELD *field, int type, const char *name, int len, int dec)
{
	memset(field, 0, sizeof(DB_FIELD));
	field->field_type = type;
	strncpy(field->field_name, name, 11);
	field->field_length = len;
	field->field_decimals = dec;
	return 0;
}
/* }}} */

/******************************************************************************
	Block with functions to read out special dBASE information, like
		- date
		- usage of memo
 ******************************************************************************/

/* dbf_GetDate() {{{
 * Returns the date of last modification as a human readable string.
 */
const char *dbf_GetDate(P_DBF *p_dbf)
{
	static char date[10];

	if ( p_dbf->header->last_update[0] ) {
		sprintf(date, "%d-%02d-%02d",
		1900 + p_dbf->header->last_update[0], p_dbf->header->last_update[1], p_dbf->header->last_update[2]);

		return date;
	} else {
		perror("In function GetDate(): ");
		return "";
	}

	return 0;
}
/* }}} */

/* dbf_HeaderSize() {{{
 */
int dbf_HeaderSize(P_DBF *p_dbf)
{
 	if ( p_dbf->header->header_length > 0 ) {
		return p_dbf->header->header_length;
	} else {
		perror(_("In function dbf_HeaderSize(): "));
		return -1;
	}

	return 0;
}
/* }}} */

/* dbf_RecordLength() {{{
 * Returns the length of a record.
 */
int dbf_RecordLength(P_DBF *p_dbf)
{
 	if (p_dbf->header->record_length > 0) {
		return p_dbf->header->record_length;
	} else {
		perror(_("In function dbf_RecordLength(): "));
		return -1;
	}

	return 0;
}
/* }}} */

/* dbf_GetStringVersion() {{{
 * Returns the verion of the dbase file as a human readable string.
 */
const char *dbf_GetStringVersion(P_DBF *p_dbf)
{
	if ( p_dbf->header->version == 0 ) {
		perror(_("In function dbf_GetStringVersion(): "));
		return (char *)-1;
	}

	return get_db_version(p_dbf->header->version);
}
/* }}} */

/* dbf_GetVersion() {{{
 * Returns the verion field as it is storedi in the header.
 */
int dbf_GetVersion(P_DBF *p_dbf)
{
	if ( p_dbf->header->version == 0 ) {
		perror(_("In function dbf_GetVersion(): "));
		return -1;
	}

	return p_dbf->header->version;
}
/* }}} */

/* dbf_IsMemo() {{{
 */
int dbf_IsMemo(P_DBF *p_dbf)
{
	int memo;

	if ( p_dbf->header->version == 0 ) {
		perror(_("In function dbf_IsMemo(): "));
		return -1;
	}

	memo = (p_dbf->header->version  & 128)==128 ? 1 : 0;

	return memo;
}
/* }}} */

/******************************************************************************
	Block with functions to read records
 ******************************************************************************/

/* dbf_SetRecordOffset() {{{
 */
int dbf_SetRecordOffset(P_DBF *p_dbf, int offset) {
	if(offset == 0)
		return -3;
	if(offset > (int) p_dbf->header->records)
		return -1;
	if((offset < 0) && (abs(offset) > p_dbf->header->records))
		return -2;
	if(offset < 0)
		p_dbf->cur_record = (int) p_dbf->header->records + offset;
	else
		p_dbf->cur_record = offset-1;
	return p_dbf->cur_record;
}
/* }}} */

/* dbf_ReadRecord() {{{
 */
int dbf_ReadRecord(P_DBF *p_dbf, char *record, int len) {
	off_t offset;

	if(p_dbf->cur_record >= p_dbf->header->records)
		return -1;

	offset = lseek(p_dbf->dbf_fh, p_dbf->header->header_length + p_dbf->cur_record * (p_dbf->header->record_length), SEEK_SET);
//	fprintf(stdout, "Offset = %d, Record length = %d\n", offset, p_dbf->header->record_length);
	if (read( p_dbf->dbf_fh, record, p_dbf->header->record_length) == -1 ) {
		return -1;
	}
	p_dbf->cur_record++;
	return p_dbf->cur_record-1;
}
/* }}} */

/* dbf_WriteRecord() {{{
 */
int dbf_WriteRecord(P_DBF *p_dbf, char *record, int len) {

	if(len != p_dbf->header->record_length-1) {
		fprintf(stderr, _("Length of record mismatches expected length (%d != %d)."), len, p_dbf->header->record_length);
		fprintf(stderr, "\n");
		return -1;
	}
	lseek(p_dbf->dbf_fh, 0, SEEK_END);
	if (write( p_dbf->dbf_fh, " ", 1) == -1 ) {
		return -1;
	}
	if (write( p_dbf->dbf_fh, record, p_dbf->header->record_length-1) == -1 ) {
		return -1;
	}
	p_dbf->header->records++;
	if(0 > dbf_WriteHeaderInfo(p_dbf, p_dbf->header)) {
		return -1;
	}
	return p_dbf->header->records;
}
/* }}} */

/* dbf_GetRecordData() {{{
 */
char *dbf_GetRecordData(P_DBF *p_dbf, char *record, int column) {
	return(record + p_dbf->fields[column].field_offset);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
