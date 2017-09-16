/*
 * config_parse_sample.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: cgi0911
 */

/* To be modified:
 * 1. Add support to rule ID.
 * 2. Use inet_pton, not self-defined parsing.
 * 3. Check carefully the support to empty matching strings.
 */


/*
 * ==== Include files ====
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <arpa/inet.h>

/*
 * ==== Macros and using namespace ====
 */
//#define DEBUG       1           // Comment out and no message will show.
#define MAX_PORT    65535       // Max port number (min is 0)
#define MAX_STRING  1000        // Max length of matching string

using namespace std;


/*
 * ==== User-defined data structures ====
 */

struct cidr_t{
    unsigned char   buf[sizeof(struct in_addr)];
                                         /*
                                          * IP converted into an array of
                                          * unsigned bytes. Note that unsigned
                                          * char is actually used as 1-byte
                                          * integer here.
                                          */
    unsigned int    pre_len;
                                         /*
                                          * Prefix length.
                                          * Should be 0 <= pre_len <= 32
                                          */
}; /* Parse char[] into this data structure */

struct port_t{
    unsigned int    upper;
    unsigned int    lower;
}; /* Parse char[] into this port range structure */

struct rule_t{
    int             id;
    cidr_t *        src_ip;
    port_t *        src_port;
    cidr_t *        dst_ip;
    port_t *        dst_port;
    char *          protocol;
    char *          match_str;
}; /* Data structure for storing the whole rule */


// ==== Function prototypes ====
cidr_t* parse_cidr(char * in_str);
port_t* parse_port(char * in_str);
char*   parse_protocol(char * in_str);
char*   parse_match_string(char * in_str);

// ==== Main course ====
int main( int argc, char* argv[] ) {
    string line;
    ifstream config_file( argv[1] );
    int rid = 1;

    vector<struct rule_t> rule_vec;

    if ( config_file.is_open() ) {
        while ( getline( config_file, line ) ) {
            stringstream strs(line);
            rule_t rule;
            char  src_ip_str[20]        = "";
            char  src_port_str[20]      = "";
            char  dst_ip_str[20]        = "";
            char  dst_port_str[20]      = "";
            char  protocol_str[20]      = "";
            char  match_str[MAX_STRING] = "";

            strs >> src_ip_str >> src_port_str >> dst_ip_str >> dst_port_str
                 >> protocol_str;
            strs.getline( match_str, MAX_STRING );

            /* Stop if line is empty. */
            if(strcmp(src_ip_str, "\0") == 0) { break; }

            rule.id = rid;

#ifdef DEBUG
            cout << "Rule ID: " << rule->id << endl;
#endif

            rule.src_ip = parse_cidr(src_ip_str);
            rule.src_port = parse_port(src_port_str);
            rule.dst_ip = parse_cidr(dst_ip_str);
            rule.dst_port = parse_port(dst_port_str);
            rule.protocol = parse_protocol(protocol_str);
            rule.match_str = parse_match_string(match_str);

            /*
             * We parse each line of the configuration file into a rule
             * strucutre, and push it to the back of a vector. It is up to
             * you to make use of this vector.
             */
            rule_vec.push_back(rule);

            rid ++;

#ifdef DEBUG
            cout << endl;
#endif
        }
    }

    /*
     * Print the content of rules (after parsing), one by one.
     */

    vector<struct rule_t>::iterator it;
    cout << endl << "Rules in the rule vector (after parsing): " << endl;

    for(it = rule_vec.begin() ; it < rule_vec.end() ; it++){
        cout << "#";
        cout << it->id << "  ";
        cout << (int) it->src_ip->buf[0]  << "." <<
                (int) it->src_ip->buf[1]  << "." <<
                (int) it->src_ip->buf[2]  << "." <<
                (int) it->src_ip->buf[3]  << "/" <<
                it->src_ip->pre_len       << "  ";
        cout << it->src_port->lower << ":" <<
                it->src_port->upper << "  ";
        cout << (int) it->dst_ip->buf[0]  << "." <<
                (int) it->dst_ip->buf[1]  << "." <<
                (int) it->dst_ip->buf[2]  << "." <<
                (int) it->dst_ip->buf[3]  << "/" <<
                it->dst_ip->pre_len       << "  ";
        cout << it->dst_port->lower << ":" <<
                it->dst_port->upper << "  ";
        cout << it->protocol        << "  ";
        cout << "\"" << it->match_str << "\"" << endl;
    }

    return 0;
}

/*
 * cidr_t * parse_cidr(char * in_str)
 * Parse an input char[] into CIDR data type.
 * If in_str is not a legal representation of CIDR, return 0.0.0.0/0, i.e.
 * all-wildcard prefix.
 */
cidr_t * parse_cidr(char * in_str){
    unsigned int   i;
    unsigned char   temp_buf[sizeof(struct in_addr)] = {0};
    int             prefix_length;
    char *          str_ip;
    char *          str_plength;
    int             wildcard = 0;

    cidr_t * res = (cidr_t *) malloc(sizeof(cidr_t));

    if(strcmp(in_str, "*") == 0) { wildcard = 1; }

    else{
        /*
         * Split in_str into two parts: the part before '/' should be a valid
         * IPv4 expression, and the part after '/' should be an integer
         * between 0 and 32.
         */
        str_ip = strtok(in_str, "/");
        str_plength = strtok(NULL, "");

        /*
         * Parse and check validity of str_ip.
         */

        int valid_ip = inet_pton(AF_INET, str_ip, temp_buf);

        /*
         * If the IP string is not a valid IPv4 expression, we will return a
         * wildcard parsing result (0.0.0.0/0).
         */
        if(valid_ip <= 0) { wildcard = 1; }

        /*
         * Parse and check validity of str_plength
         */
        if ( str_plength == NULL ) {
			prefix_length = 32;
		} else {
            char * ptr;
            strtol(str_plength, &ptr, 10);
            if( *ptr != 0 ) wildcard = 1;
                        /*
                         * str_plength not in numerical form
                         */
            else {
                prefix_length = atoi(str_plength);
                /*
                 * Check if the number represented by str_plength is a valid
                 * prefix length number.
                 */
                if(prefix_length > 32 || prefix_length < 0) { wildcard = 1; }
            }
        }
    }

    /*
     * Copy values to res.
     */
    if ( wildcard == 0 ) {
        for( i = 0; i < sizeof(struct in_addr); i++ ) {
            res->buf[i] = temp_buf[i];
        }
        res->pre_len = prefix_length;
    } else {
        for( i = 0; i < sizeof(struct in_addr); i++ ) {
            res->buf[i] = 0;
        }
        res->pre_len = 0;
    }

#ifdef DEBUG
    cout << "Parsed CIDR: ";
    for(i=0 ; i<sizeof(struct in_addr) - 1 ; i++){
        printf("%d.", res->buf[i]);
    }
    printf("%d/", res->buf[sizeof(struct in_addr)-1]);
    printf("%d\n", res->pre_len);
#endif

    return res;

}

/*
 * port_t * parse_port(char * in_str)
 * Parse an input char[] into port range data type.
 * If not a legal expression of port range, return 0:65535
 */
port_t * parse_port(char * in_str){
    int i;
    char* tok_strs[2];
    unsigned long int tok_nums[2];
    int legal = 1;
    int exact_match = 0;
    port_t * res = (port_t *) malloc(sizeof(port_t));

    /* If in_str = "*", we will return all-wildcard. */
    if(strcmp(in_str, "*") == 0)    legal = 0;

    /* Initialize token strings. */
    for(i=0 ; i<2 ; i++)    tok_strs[i] = NULL;

    /* The case where in_str starts with ":" */
    if(in_str[0] == ':'){
        tok_strs[1] = strtok(in_str + 1, "");
        tok_nums[0] = 0;

        /* tok_strs[1] must be a 10-base integer. */
        char * ptr;
        strtol(tok_strs[1], & ptr, 10);
        if(*ptr != 0)            legal = 0;
        else{
            tok_nums[1] = atoi(tok_strs[1]);
            if(tok_nums[1] < 0 || tok_nums[1] > MAX_PORT)     legal = 0;
        }
    }


    /* The case where in_str ends with ":" */
    else if(in_str[strlen(in_str) - 1] == ':'){
        tok_strs[0] = strtok(in_str, ":");
        tok_nums[1] = MAX_PORT;

        /* tok_strs[0] must be a 10-base integer. */
        char * ptr;
        strtol(tok_strs[0], &ptr, 10);
        if(*ptr != 0)           legal = 0;
        else{
            tok_nums[0] = atoi(tok_strs[0]);
            if(tok_nums[0] < 0 || tok_nums[0] > MAX_PORT)   legal = 0;
        }
    }

    /* Other cases*/
    else{
        tok_strs[0] = strtok(in_str, ":");
        tok_strs[1] = strtok(NULL, "");

        if(tok_strs[1] == NULL) { exact_match = 1; }

        /* tok_strs[0-1] must be a 10-base integer. */
        for(i=0 ; i<2 ; i++){
            if(tok_strs[i] != NULL)
            {
                char * ptr;
                strtol(tok_strs[i], &ptr, 10);
                if(*ptr != 0)           legal = 0;
                else{
                    tok_nums[i] = atoi(tok_strs[i]);
                    if(tok_nums[i] < 0 || tok_nums[i] > MAX_PORT)   legal = 0;
                }
            }
        }

        if(exact_match == 1)    { tok_nums[1] = tok_nums[0]; }
        else if(tok_nums[0] > tok_nums[1])  { legal = 0; }
    }


    /*
     * If the in_str is a legal CIDR expression, copy the parsed numerical data
     * to return data structure. If not legal, return 0.0.0.0/0.
     */
    if(legal){
        res->lower = tok_nums[0];
        res->upper = tok_nums[1];
    }

    else{
        res->lower = 0;
        res->upper = MAX_PORT;
    }

#ifdef DEBUG
    cout << "Parsed port range: ";
    cout << res->lower      << ":"   <<
            res->upper      << endl  ;
#endif

    return res;
}




/*
 * char * parse_protocol(char * in_str)
 * Parse an input char[] into protocol string.
 * Input is case-insensitive, but the output protocol name will be in lower-case.
 * If not a legal expression of protocol, return "*", i.e. wildcard.
 */
char * parse_protocol(char * in_str){
    int  i;
    char * res = (char *) malloc(10 * sizeof(char));
    char temp[strlen(in_str)];

    for(i=0 ; temp[i] ; i++){
        temp[i] = tolower(in_str[i]);
    }

    if(strcmp(temp, "*") == 0)          { strcpy(res, "*"); }

    else if(strcmp(temp, "tcp") == 0)   { strcpy(res, "tcp"); }

    else if(strcmp(temp, "udp") == 0)   { strcpy(res, "udp"); }

    else if(strcmp(temp, "icmp") == 0)  { strcpy(res, "icmp"); }

    else{ strcpy(res, "*"); }

#ifdef DEBUG
    cout << "Parsed protocol: ";
    cout << res << endl;
#endif

    return res;
}




/*
 * char * parse_match_string(char * in_str)
 * Parse an input char[] into matching string.
 * Input string can include spaces, but shall be enclosed in " ".
 */
char * parse_match_string(char * in_str){
    int  i;
    int  n_st_sp = 0;       /* Number of leading white spaces */
    int  n_ed_sp = 0;       /* Number of trailing white spaces */
    int  n_chars = 0;
    int  len = strlen(in_str);
    char * res = (char *) malloc(sizeof(char) * MAX_STRING);


    /*
     * Count the number of leading and trailing white spaces of in_str.
     */
    for(i=0 ; i<len ; i++){
        if(in_str[i] == ' ') { n_st_sp ++; }
        else break;
    }

    for(i=len-1 ; i>=0 ; i--){
        if(in_str[i] == ' ') { n_ed_sp ++; }
        else break;
    }


    /*
     * Check validity of the string, which means, after removing leading and
     * trailing white spaces, the rest of in_str must be enclosed in " ". If
     * not, in_str is not a legal expression, and we shall return a null
     * matching string.
     */

    /*
     * If the part of string after trimming is less than two characters long,
     * then the string must be illegal.
     */
    if(n_st_sp >= len - n_ed_sp - 1){
        strcpy(res, "");
    }

    /*
     * Check if the trimmed string is enclosed in quotation marks.
     */
    else if(in_str[n_st_sp] != '\"' ||
            in_str[len - n_ed_sp - 1] != '\"'){
        strcpy(res, "");
    }

    /*
     * Extract the string between quotation marks.
     */
    else{
        n_chars = len - n_st_sp - n_ed_sp - 2;
        if(n_chars == 0){
            strcpy(res, "");
        }
        else{
            strncpy(res, in_str + (n_st_sp+sizeof(char)), n_chars);
        }
    }

#ifdef DEBUG
    cout << "Parsed matching string (quote marks truncated): ";
    cout << res << endl ;
#endif

    return res;
}
