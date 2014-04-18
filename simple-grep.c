#include <stdio.h>
#include <pcre.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

const char *program_name = NULL;

/* If size of allocated memory less then needed memory, function return reallocated memory of double size, else
return original pointer. These function let us append data in string. */
char* check_and_realloc_twice(char *pointer, size_t *size, size_t new_size) {
    if (*size < new_size) {
        char *temp_pointer;
        /* We don't want reallocate memory often, so we get double size of needed memory */
        *size = new_size * 2;
        temp_pointer = realloc(pointer, *size * sizeof(char));
        if (temp_pointer != NULL) {
            pointer = temp_pointer;
        } else {
            exit(1);
        }
    }
    return pointer;
}

/* Set \Q and \E around plain text for catch regular expressions from stdin and write to target */
char* append_plain_text(char *target, size_t *target_size, char *source, size_t source_size) {
    if (source_size > 0) {
        size_t length;
        length = strlen(target);
        target = check_and_realloc_twice(target, target_size, length+source_size+5);
        strncpy(target+length, "\\Q", 2);
        strncpy(target+length+2, source, source_size);
        strncpy(target+length+2+source_size, "\\E", 2);
        *(target+length+4+source_size) = '\0';    
    }
    return target;
}

/* Simple token parse function. Walk character by character and replace token capture by 
valid PCRE regexp. */
char* convert_pattern(char *pattern, int *token_count) {
    size_t i;
    size_t offset = 0, p_offset;
    size_t length;
    size_t pattern_length = strlen(pattern);
    size_t token_size = 10, spaces_size = 10, pattern_size = pattern_length + 1, result_size = pattern_length + 1;
    char *result, *token_index, *new_pattern, *spaces_count;

    char g_template[] = "(?<%s>\\b.+\\b)";
    char s_template[] = "(?<%s>(?:\\w* \\w*){%s})";
    char o_template[] = "(?<%s>\\b.+?\\b)";

    /* Option capture modifier: S# and G */
    int spaces, greedy;

    result = (char *)malloc(result_size * sizeof(char));
    new_pattern = (char *)malloc(pattern_size * sizeof(char));
    token_index = (char *)malloc(token_size * sizeof(char));
    spaces_count = (char *)malloc(spaces_size * sizeof(char));

    /* If we can't allocate memory, return NULL and exit */
    if (new_pattern == NULL || result == NULL || token_index == NULL || spaces_count == NULL) {
        printf("Out of memory");
        return NULL;
    }

    result[0] = '^';
    result[1] = '\0';
    token_index[0] = '\0';
    new_pattern[0] = '\0';
    
    /* parse pattern character by character. If we find valid token than we replace it by regexp pattern */
    for (i = 0; i < pattern_length; i++) {
        /* p_offset is a offset inside pattern */
        p_offset = 0;
        greedy = 0;
        spaces = 0;

        /* All plain text must be isolated by \Q and \E, but if \E will be isolated too, we get error, avoid its isolation. */
        if (*(pattern+i) == '\\') {
            if (*(pattern+i+1) == 'E') {
                result = append_plain_text(result, &result_size, pattern+offset, i-offset);
                length = strlen(result);
                strncpy(result+length, "\\\\E", 3);
                *(result+length+3) = '\0';
                /* Skip symbol 'E', offset must be set at next place after E */
                i = i+1;
                offset = i+1;
            }
        }

        if (*(pattern+i) != '%') {
            continue;
        }
        p_offset++;
        if (*(pattern+i+p_offset) != '{') {
            i = i + p_offset;
            continue;
        }
        p_offset++;

        /* Put non-negative integer in token_index */
        while((*(pattern+i+p_offset) >= '0' && (*(pattern+i+p_offset)) <= '9')) {
            token_index = check_and_realloc_twice(token_index, &token_size, p_offset-2);
            *(token_index + p_offset - 2) = *(pattern+i+p_offset);
            p_offset++;
        }
        *(token_index + p_offset - 2) = '\0';
        
        /* Token index is not optional, check for length of non-negative integer string */
        if (strlen(token_index) == 0) {
            continue;
        } 

        /* Optional capture modifier */
        if (*(pattern+i+p_offset) == 'S') {
            p_offset++;
            spaces = 1;
            length = p_offset;

            /* Getting the number of spaces */
            while((*(pattern+i+p_offset) >= '0' && (*(pattern+i+p_offset)) <= '9')) {
                spaces_count = check_and_realloc_twice(spaces_count, &spaces_size, p_offset - length);
                *(spaces_count + p_offset - length) = *(pattern+i+p_offset);
                p_offset++;
            }
            *(spaces_count + p_offset - length) = '\0';

            /* Number of spaces is not optional */
            if (strlen(spaces_count) == 0) {
                i = i + p_offset;
                continue;
            }
        } else {
            /* Check for greedy capture modifier */
            if (*(pattern+i+p_offset) == 'G') {
                p_offset++;
                greedy = 1;
            }    
        }

        if (*(pattern+i+p_offset) != '}') {
            i = i + p_offset;
            continue;
        }

        /* Token count shows how many token we have: it's needed for allocation memory for ovector */
        *token_count = *token_count+1;

        /* Substitute variables to *_pattern and write to new_pattern */
        if (spaces > 0) {
            int sp_n = strtol(spaces_count, NULL, 10);
            if (sp_n == 0) {
                /* If spaces modifier equiv 0, capture the word */
                new_pattern = check_and_realloc_twice(new_pattern, &pattern_size, 
                    strlen(s_template)+strlen(token_index)+strlen(spaces_count)+1);
                sprintf(new_pattern, "(?<%s>\\b\\w+\\b)", token_index);    
            } else {
                new_pattern = check_and_realloc_twice(new_pattern, &pattern_size, 
                    strlen(s_template)+strlen(token_index)+strlen(spaces_count)+1);
                sprintf(new_pattern, s_template, token_index, spaces_count);    
            }
            
        } else if (greedy > 0) {
            new_pattern = check_and_realloc_twice(new_pattern, &pattern_size, 
                strlen(token_index) + strlen(g_template));
            sprintf(new_pattern, g_template, token_index);
        } else {
            new_pattern = check_and_realloc_twice(new_pattern, &pattern_size, 
                strlen(token_index) + strlen(o_template));
            sprintf(new_pattern, o_template, token_index);
        }
    
        if (i-offset) {
            /* Copy text without patterns to result string */
            result = append_plain_text(result, &result_size, pattern+offset, i-offset);
        }
        length = strlen(result);
        result = check_and_realloc_twice(result, &result_size, length+strlen(new_pattern)+1);
        strncpy(result+length, new_pattern, strlen(new_pattern)+1);
        /* Save index of last pattern symbol for copy plain text since this */
        offset = i + p_offset + 1;
        /* Next iteration will start at the end of pattern */
        i = i + p_offset;
    }

    /* Copy all symbols from offset to last symbol of pattern, because there is no tokens in tail */
    if (pattern_length-offset > 0) {
        result = append_plain_text(result, &result_size, pattern+offset, pattern_length-offset);
    }
    length = strlen(result);
    result = check_and_realloc_twice(result, &result_size, length+2);
    *(result+length) = '$';
    *(result+length+1) = '\0';
    
    /* To catch end of string by non-greedy subpattern, we must set dollar sign to end of regexp */
    free(token_index); 
    free(spaces_count);
    free(new_pattern);
    return result;
}

void usage(int status) {
    printf("Usage: %s [OPTION] PATTERN\n", program_name);
    printf("Search for PATTERN in standard input.\n\
    Output control: \n\
        -v              show content of tokens\n\
        -h              display this help and exit\n");
    exit(status);
}

int main(int argc, char **argv) {
    /* PCRE */
    int erroffset;
    int namecount;
    int name_entry_size;
    int *ovector;
    int rc;
    int subject_length;
    int veccount = 0;
    size_t content_size = 0;
    char *pattern = NULL;
    char *subject = NULL;
    const char *error;
    unsigned char *name_table;
    pcre *re;
    pcre_extra *sd;

    /* GETOPT */
    int c;
    int verbose_flag = 0;
    
    /* Save program name for usage showing */
    program_name = argv[0];

    if (argc < 2) {
        usage(1);
    }
    
    /* Check for arguments. -v is optional parameter, PATTERN is mandatory */
    while ((c = getopt(argc, argv, "vh")) != -1) {
        switch (c) {
            case 'v':
                if (argc != 3) {
                    usage(1);
                }
                verbose_flag = 1;
                break;
            case 'h':
                usage(0);
                break;
            default:
                usage(1);
        }
    }

    /* Modify command line argument - pattern into a regular expression */
    pattern = convert_pattern(argv[optind], &veccount);
    if (pattern == NULL) {
        return 1;
    }

    /* ovector will store index of our named substrings, we must allocate memory for them */
    ovector = (int *)malloc(sizeof(int)*(veccount+1)*3); 
    if (ovector == NULL) {
        return 1;
    }

    /* Allow duplicates is more effective way. Check for uniqueness is log(N) in best case. So, allow DUPNAMES. 
    In this case will be created two TCS of the same name and print if verbose_flag is True */
    re = pcre_compile(
        pattern,
        PCRE_DUPNAMES,
        &error,
        &erroffset,
        NULL);

    /* This action is not necessary, function finds additional information for pcre_exec */
    sd = pcre_study(
        re,
        0,
        &error);

    while ((subject_length = getline(&subject, &content_size, stdin)) != -1) {
        rc = pcre_exec(
            re,
            sd,
            subject,
            subject_length,
            0,
            0,
            ovector,
            (veccount+1) * 3);

        if (rc < 0) {
            switch(rc) {
                case PCRE_ERROR_NOMATCH: continue;
                default: printf("Matching error %d\n", ovector[0]);
            }
            pcre_free_study(sd);
            pcre_free(re);
            free(subject);
            free(pattern);
            free(ovector);
            return 1;
        }

        pcre_fullinfo(
            re,                   
            NULL,                 
            PCRE_INFO_NAMECOUNT,  
            &namecount);          

        /* This check will false if there isn't TCS, because convert_pattern
        replace token by named subpatterns */
        if (namecount <= 0) {
            fputs(subject, stdout);
            continue;
        } else {
            int i, j, is_valid = 1;
            unsigned char *tabptr;
            char *content[namecount], *names[namecount];

            /* Before access to substrings, we must extract the table for
            translating names to numbers, and the size of each entry in the table. */
            pcre_fullinfo(
                re,                       
                NULL,                     
                PCRE_INFO_NAMETABLE,      
                &name_table);             

            pcre_fullinfo(
                re,                       
                NULL,                     
                PCRE_INFO_NAMEENTRYSIZE,  
                &name_entry_size);

            /* Now we can scan the table and, create arrays of name and content. */
            tabptr = name_table;
            for (i = 0; i < namecount; i++) {
                int n = (tabptr[0] << 8) | tabptr[1];
                
                /* create array of TCS index */
                names[i] = (char *)malloc((name_entry_size - 3 + 2)*sizeof(char));
                strncpy(names[i], (char* )tabptr + 2, name_entry_size - 3);
                *(names[i]+name_entry_size-3) = '\0';
                
                /* create array of TCS content */
                content[i] = (char *)malloc((*(ovector + 2*n+1) - *(ovector + 2*n) + 1)*sizeof(char));
                strncpy(content[i], subject + *(ovector + 2*n), *(ovector + 2*n+1) - *(ovector + 2*n));
                *(content[i]+*(ovector + 2*n+1) - *(ovector + 2*n)) = '\0';

                /* If there are a same TCS's indexes their content must be the same. This functional allow us compare token
                inside the program without parsing output. */
                for (j = i-1; j >= 0; j--) {
                    if (strcmp(names[i], names[j]) == 0) {
                        if (strcmp(content[i],content[j]) != 0) {
                            is_valid = 0;
                            break;
                        } else {
                            strncpy(names[j], "-1", 3);
                        }
                    } else if (strcmp(names[i], names[j]) < 0) {
                        printf("break\n");
                        break;
                    }
                }

                tabptr += name_entry_size;
            }

            /* If -v set, show token content */
            if (is_valid) {
                fputs(subject, stdout);
                if (verbose_flag) {
                    printf("\tTokens:\n");
                    for (i = 0; i < namecount; i++) {
                        if (strcmp(names[i], "-1") != 0) {
                            printf("\t\t%s: '%s'\n", names[i], content[i]);
                        }
                    }    
                }
            }
            for (i = 0; i < namecount; i++) {
                free(names[i]);
                free(content[i]);
            }
        }
    }
    pcre_free_study(sd);
    pcre_free(re);
    free(subject);
    free(pattern);
    free(ovector);
    return 0;
}
