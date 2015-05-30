#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "freexl.h"

static void print_notab_string (const char *string)
{
    printf("%s", string);
}

const void* print_cell(unsigned short col, const void* handle, unsigned int row)
{
    FreeXL_CellValue cell;
    int ret = freexl_get_cell_value (handle, row, col, &cell);
    if (ret != FREEXL_OK) {
        fprintf (stderr, "CELL-VALUE-ERROR (r=%u c=%u): %d\n", row, col, ret);
        return 0;
    }

    if (col > 0)
        printf("\t");

    switch (cell.type)
    {
        case FREEXL_CELL_INT:
            printf ("%d", cell.value.int_value);
            break;
        case FREEXL_CELL_DOUBLE:
            printf ("%1.12f", cell.value.double_value);
            break;
        case FREEXL_CELL_TEXT:
        case FREEXL_CELL_SST_TEXT:
            print_notab_string (cell.value.text_value);
            break;
        case FREEXL_CELL_DATE:
        case FREEXL_CELL_DATETIME:
        case FREEXL_CELL_TIME:
            printf ("%s", cell.value.text_value);
            break;
        case FREEXL_CELL_NULL:
        default:
            printf ("NULL");
            break;
    }
    return handle;
}

const void* print_row(unsigned int row, const void* handle, unsigned short columns)
{
    for (unsigned short col = 0; col < columns; col++) {
        if (!print_cell(col, handle, row))
            return 0;
    }
    return handle;
}

const void* print_table(const void* handle, unsigned int worksheet_index)
{
    int ret;
    unsigned int rows;
    unsigned short columns;
    unsigned int row;
    unsigned short col;

    //const char *utf8_worsheet_name;
    //ret = freexl_get_worksheet_name (handle, worksheet_index, &utf8_worsheet_name);
    //if (ret != FREEXL_OK) {
    //    fprintf (stderr, "GET-WORKSHEET-NAME Error: %d\n", ret);
    //    return 0;
    //}

    ret = freexl_select_active_worksheet (handle, worksheet_index);
    if (ret != FREEXL_OK) {
        fprintf (stderr, "SELECT-ACTIVE_WORKSHEET Error: %d\n", ret);
        return 0;
    }

    ret = freexl_worksheet_dimensions (handle, &rows, &columns);
    if (ret != FREEXL_OK)
    {
        fprintf (stderr, "WORKSHEET-DIMENSIONS Error: %d\n", ret);
        return 0;
    }

    for (row = 0; row < rows; row++)
    {
        if (!print_row(row, handle, columns))
            return 0;
        printf("\n");
    }

    return handle;
}

int
main (int argc, char *argv[])
{
    if (argc < 2) {
        fprintf (stderr, "usage: xl2sql path.xls [table_prefix]\n");
        return -1;
    }

    unsigned int worksheet_index;
    unsigned int info;
    unsigned int max_worksheet;
    const void *handle;
    int ret;
    if ( (ret = freexl_open (argv[1], &handle)) != FREEXL_OK) {
        fprintf (stderr, "OPEN ERROR: %d\n", ret);
        return -1;
    }

    if ( (ret = freexl_get_info (handle, FREEXL_BIFF_PASSWORD, &info)) != FREEXL_OK) {
        fprintf (stderr, "GET-INFO [FREEXL_BIFF_PASSWORD] Error: %d\n", ret);
        return 0;
    }
    switch (info) {
        case FREEXL_BIFF_PLAIN:
            break;
        case FREEXL_BIFF_OBFUSCATED:
        default:
            fprintf (stderr, "Password protected: (not accessible)\n");
            return -1;
    };

    if ( (ret = freexl_get_info (handle, FREEXL_BIFF_SHEET_COUNT, &max_worksheet)) != FREEXL_OK) {
        fprintf (stderr, "GET-INFO [FREEXL_BIFF_SHEET_COUNT] Error: %d\n", ret);
        return -1;
    }

    for (worksheet_index = 0; worksheet_index < max_worksheet; worksheet_index++) {
        print_table(handle, worksheet_index);
    }

    freexl_close (handle);
}

