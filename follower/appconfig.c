/* $Id$ */
#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>

#include "appconfig.h"

int g_config_listen_port = 0;
sc_config_pattern_entry* g_config_patterns = NULL;

/*
int
_pick_mapping(yaml_parser_t* parser, int lv)
{
    int done = 0;
        int error = 0;

        yaml_event_t event;
        while (!done)
        {
            if (!yaml_parser_parse(parser, &event)) {
                error = 1;
                break;
            }

            switch (event.type) {
            case YAML_SCALAR_EVENT:
                printf("%d) event.data.scalar.value = [%s]\n",
                lv, event.data.scalar.value);
                break;
            case YAML_SEQUENCE_START_EVENT:
                // printf("event.data.sequence_start.anchor = [%s] .tag = [%s]\n",
                // event.data.sequence_start.anchor, event.data.sequence_start.tag);
                _pick_sequence(parser, lv + 1);
                break;
            case YAML_MAPPING_START_EVENT:
                printf("%d) event.data.mapping_start.anchor = [%s] .tag = [%s]\n",
                lv, event.data.mapping_start.anchor, event.data.mapping_start.tag);
                _pick_mapping(parser, lv + 1);
                break;
            case YAML_MAPPING_END_EVENT:
                yaml_event_delete(&event);
                return 0;
                break;
            default:
                printf("event.type = %d\n", event.type);
                break;
            }
            done = (event.type == YAML_STREAM_END_EVENT);

            yaml_event_delete(&event);
        }

        return -1;
}
*/

sc_config_pattern_entry*
_pick_pattern_entry(yaml_parser_t* parser)
{
    sc_config_pattern_entry* ret =
    (sc_config_pattern_entry*)malloc(sizeof(sc_config_pattern_entry));

    int done = 0;
    int error = 0;
    yaml_event_t event, event_value;

    memset(ret, 0, sizeof(sc_config_pattern_entry));

    while (!done) {
        if (!yaml_parser_parse(parser, &event)) {
            break;
        }

        switch (event.type) {
        case YAML_SCALAR_EVENT:
            if (!yaml_parser_parse(parser, &event_value)) {
                return ret;
            }
            if (strcmp(event.data.scalar.value, "path") == 0) {
                ret->path = strdup(event_value.data.scalar.value);
            } else if (strcmp(event.data.scalar.value, "displayName") == 0) {
                ret->displayName = strdup(event_value.data.scalar.value);
            } else if (strcmp(event.data.scalar.value, "rotate") == 0) {
                ret->rotate = strcasecmp(event_value.data.scalar.value, "true")
                == 0 ? 1 : 0;
            }
            yaml_event_delete(&event_value);
            break;

        case YAML_MAPPING_END_EVENT:
            yaml_event_delete(&event);
            return ret;

        default:
            printf("event.type = %d\n", event.type);
            break;
        }

        yaml_event_delete(&event);
    }

    return ret;
}

sc_config_pattern_entry *
_pick_patterns(yaml_parser_t* parser)
{
    sc_config_pattern_entry *list = NULL;
    int done = 0;
    int error = 0;

    yaml_event_t event;
    while (!done) {
        if (!yaml_parser_parse(parser, &event)) {
            error = 1;
            break;
        }

        switch (event.type) {
        /*
        case YAML_SCALAR_EVENT:
            printf("%d) event.data.scalar.value = [%s]\n",
            lv, event.data.scalar.value);
            break;
        case YAML_SEQUENCE_START_EVENT:
            // printf("event.data.sequence_start.anchor = [%s] .tag = [%s]\n",
            // event.data.sequence_start.anchor, event.data.sequence_start.tag);
            _pick_sequence(parser, lv + 1);
            break;
            */
        case YAML_SEQUENCE_END_EVENT:
            yaml_event_delete(&event);
            return list;

        case YAML_MAPPING_START_EVENT:
            {
                sc_config_pattern_entry *entry = _pick_pattern_entry(parser);
                if (entry) {
                    entry->_next = list;
                    list = entry;
                }
            }
            break;
            /*
        case YAML_MAPPING_END_EVENT:
            yaml_event_delete(&event);
            return 0;
            */
        default:
            printf("%s: event.type = %d\n", __FUNCTION__, event.type);
            break;
        }

        yaml_event_delete(&event);
    }

    return list;
}

int
_pick_global(yaml_parser_t* parser)
{
    int done = 0;
    int error = 0;
    int st = 0;

    yaml_event_t event, evvalue;
    while (!done) {
        if (!yaml_parser_parse(parser, &event)) {
            error = 1;
            break;
        }

        if (event.type != YAML_SCALAR_EVENT) {
            error = 1;
            break;
        }

        if (strcmp(event.data.scalar.value, "port") == 0) {
            if (!yaml_parser_parse(parser, &evvalue)) {
                error = 1;
                break;
            }

            g_config_listen_port = strtoul(evvalue.data.scalar.value, NULL, 10);
        } else if (strcmp(event.data.scalar.value, "patterns") == 0) {
            g_config_patterns = _pick_patterns(parser);
        }

        switch (evvalue.type) {
            /*
        case YAML_MAPPING_START_EVENT:
            printf("%d) event.data.mapping_start.anchor = [%s] .tag = [%s]\n",
            lv, event.data.mapping_start.anchor, event.data.mapping_start.tag);
            _pick_mapping(parser, lv + 1);
            break;
        case YAML_MAPPING_END_EVENT:
            yaml_event_delete(&event);
            return 0;
        */

        default:
            printf("event.type = %d\n", event.type);
            break;
        }

        yaml_event_delete(&event);
    }

    return -1;
}

int
parse_config_file(const char* fname)
{
    yaml_parser_t parser;
    yaml_event_t event;
    int done = 0;
    int error = 0;

    FILE *file;

    file = fopen(fname, "rb");

    yaml_parser_initialize(&parser);

    yaml_parser_set_input_file(&parser, file);

    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            error = 1;
            break;
        }

        switch (event.type) {
        case YAML_SCALAR_EVENT:
            printf("event.data.scalar.value = [%s]\n",
            event.data.scalar.value);
            break;
        case YAML_SEQUENCE_START_EVENT:
            printf("event.data.sequence_start.anchor = [%s] .tag = [%s]\n",
            event.data.sequence_start.anchor, event.data.sequence_start.tag);
            break;
        case YAML_MAPPING_START_EVENT:
            printf("event.data.mapping_start.anchor = [%s] .tag = [%s]\n",
            event.data.mapping_start.anchor, event.data.mapping_start.tag);
            _pick_global(&parser);
            break;
        default:
            printf("event.type = %d\n", event.type);
            break;
        }
        done = (event.type == YAML_STREAM_END_EVENT);

        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);

    fclose(file);

    printf("%s\n", (error ? "FAILURE" : "SUCCESS"));

    return 0;
}
