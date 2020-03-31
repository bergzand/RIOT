/*
 * Copyright (C) 2019 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "native_internal.h"
#include "uri_parser.h"

char buffer[257];

int main(void)
{
	uri_parser_result_t uri_parsed;

    ssize_t len = read(STDIN_FILENO, buffer, sizeof(buffer));
    printf("Reading %ld bytes from stdin\n", (long signed)len);
    if (len < 0) {
        return -1;
    }

    memset(&uri_parsed, 0, sizeof(uri_parsed));
	if (uri_parser_process(&uri_parsed, (const char*)buffer) < 0) {
        printf("Failed to parse\n");
    }

    if (uri_parsed.scheme) {
        printf("Scheme: %.*s\n", uri_parsed.scheme_len, uri_parsed.scheme);
    }

    if (uri_parsed.userinfo) {
        printf("Userinfo: %.*s\n", uri_parsed.userinfo_len, uri_parsed.userinfo);
    }

    if (uri_parsed.host) {
        printf("Host: %.*s\n", uri_parsed.host_len, uri_parsed.host);
    }

    if (uri_parsed.port) {
        printf("Port: %.*s\n", uri_parsed.port_len, uri_parsed.port);
    }

    if (uri_parsed.path) {
        printf("Path: %.*s\n", uri_parsed.path_len, uri_parsed.path);
    }

    if (uri_parsed.query) {
        printf("Query: %.*s\n", uri_parsed.query_len, uri_parsed.query);
    }

    real_exit(0);

    return 0;
}
