/* Example code: yaml file parser using libyaml
 * author: smavros
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <yaml.h>

/* types  ------------------------------------------------------------- */

typedef struct coil {
    double freq;
    double* radius;
    double* x_center;
    double* y_center;
    unsigned int cur;
} Coil_t;

/* prototypes --------------------------------------------------------- */

/* init util */
void init_data( Coil_t* coil, char** argv );

/* global parser */
unsigned int parser( Coil_t* coil, char** argv );

/* parser util */
void init_prs( FILE** fh, yaml_parser_t* parser );
void parse_next( yaml_parser_t* parser, yaml_event_t* event );
void clean_prs( FILE** fh, yaml_parser_t* parser, yaml_event_t* event );

/* parser actions */
void event_switch( bool* seq_status, unsigned int* map_seq, Coil_t* coil,
                   yaml_parser_t* parser, yaml_event_t* event );
void to_data( bool* seq_status, unsigned int* map_seq, Coil_t* coil,
              yaml_parser_t* parser, yaml_event_t* event );
void to_data_from_map( char* buf, unsigned int* map_seq, Coil_t* coil,
                       yaml_parser_t* parser, yaml_event_t* event );

/* post parsing utils */
void parsed_loops( unsigned int* nlp, char** argv, Coil_t* coil );
void data_realloc( unsigned int nlp, Coil_t* coil );

/* printing */
void print_data( unsigned int nlps, Coil_t* coil );

/* main --------------------------------------------------------------- */

int main( int argc, char** argv )
{
    assert( argc == 2 );           /* check args number */
    assert( atoi( argv[1] ) > 0 ); /* check args validity */

    Coil_t coil; /* create my coil */

    init_data( &coil, argv ); /* initialize my coil */

    unsigned int nlp;            /* number of loops in config file */
    nlp = parser( &coil, argv ); /* parse my coil */

    parsed_loops( &nlp, argv, &coil ); /* conflict warning arg-config loops */

    print_data( nlp, &coil ); /* print the parsed coil */

    return EXIT_SUCCESS;
}

/* definitions -------------------------------------------------------- */

void parsed_loops( unsigned int* nlp, char** argv, Coil_t* coil )
{
    if ( ( *nlp ) > atoi( argv[1] ) ) {

        puts( "\n -INFO: The config file contains more coil loops than "
              "the number that was asked to be parsed in!" );
        *nlp = atoi( argv[1] );

    } else if ( ( *nlp ) < atoi( argv[1] ) ) {
        /*  In case of larger number of arg coils compared to the
         *  configuration file denoted coils one must reallocate the coil
         *  in order to avoid overflow errors over data structure
         */
        puts( "\n -INFO: The config file contains fewer coil loops than "
              "the number that was asked to be parsed in!" );
        puts( " -INFO: Parsed data structure reallocated!" );
        data_realloc( *nlp, coil ); /* data reallocation */
    }
}

void data_realloc( unsigned int nlp, Coil_t* coil )
{
    coil->radius = realloc( coil->radius, nlp * sizeof( double* ) );
    coil->x_center = realloc( coil->x_center, nlp * sizeof( double* ) );
    coil->y_center = realloc( coil->y_center, nlp * sizeof( double* ) );
}

void init_data( Coil_t* coil, char** argv )
{
    coil->radius = calloc( atoi( argv[1] ), sizeof( double* ) );
    coil->x_center = calloc( atoi( argv[1] ), sizeof( double* ) );
    coil->y_center = calloc( atoi( argv[1] ), sizeof( double* ) );
}

unsigned int parser( Coil_t* coil, char** argv )
{
    /* open file & declare libyaml types */
    FILE* fh = fopen( "data.yml", "r" );
    yaml_parser_t parser;
    yaml_event_t event;

    bool seq_status = 0;      /* IN or OUT of sequence index, init to OUT */
    unsigned int map_seq = 0; /* index of mapping inside sequence */

    init_prs( &fh, &parser ); /* initiliaze parser & open file */

    do {

        parse_next( &parser, &event ); /* parse new event */

        /* decide what to do with each event */
        event_switch( &seq_status, &map_seq, coil, &parser, &event );

        if ( event.type != YAML_STREAM_END_EVENT ) {
            yaml_event_delete( &event );
        }

        if ( map_seq > atoi( argv[1] ) ) {
            break;
        }

    } while ( event.type != YAML_STREAM_END_EVENT );

    clean_prs( &fh, &parser, &event ); /* clean parser & close file */

    return map_seq;
}

void event_switch( bool* seq_status, unsigned int* map_seq, Coil_t* coil,
                   yaml_parser_t* parser, yaml_event_t* event )
{
    switch ( event->type ) {
        case YAML_STREAM_START_EVENT:
            break;
        case YAML_STREAM_END_EVENT:
            break;
        case YAML_DOCUMENT_START_EVENT:
            break;
        case YAML_DOCUMENT_END_EVENT:
            break;
        case YAML_SEQUENCE_START_EVENT:
            ( *seq_status ) = true;
            break;
        case YAML_SEQUENCE_END_EVENT:
            ( *seq_status ) = false;
            break;
        case YAML_MAPPING_START_EVENT:
            if ( *seq_status == 1 ) {
                ( *map_seq )++;
            }
            break;
        case YAML_MAPPING_END_EVENT:
            break;
        case YAML_ALIAS_EVENT:
            printf( " ERROR: Got alias (anchor %s)\n",
                    event->data.alias.anchor );
            exit( EXIT_FAILURE );
            break;
        case YAML_SCALAR_EVENT:
            to_data( seq_status, map_seq, coil, parser, event );
            break;
        case YAML_NO_EVENT:
            puts( " ERROR: No event!" );
            exit( EXIT_FAILURE );
            break;
    }
}

void to_data( bool* seq_status, unsigned int* map_seq, Coil_t* coil,
              yaml_parser_t* parser, yaml_event_t* event )
{
    char* buf = (char*)event->data.scalar.value;
    char* cb; /* char part buffer for strtod() */

    /* dictionary */
    char* cur = "cur";
    char* freq = "freq";
    char* loops = "loops";

    if ( !strcmp( buf, cur ) ) {
        parse_next( parser, event );
        coil->cur = atoi( (char*)event->data.scalar.value );
    } else if ( !strcmp( buf, freq ) ) {
        parse_next( parser, event );
        coil->freq = strtod( (char*)event->data.scalar.value, &cb );
    } else if ( ( *seq_status ) == true ) {
        /* data from sequence of loops */
        to_data_from_map( buf, map_seq, coil, parser, event );
    } else if ( !strcmp( buf, loops ) ) {
        /* do nothing. "loops" is just the label of mapping's sequence */
    } else {
        printf( "\n -ERROR: Unknow variable in config file: %s\n", buf );
        exit( EXIT_FAILURE );
    }
}

void to_data_from_map( char* buf, unsigned int* map_seq, Coil_t* coil,
                       yaml_parser_t* parser, yaml_event_t* event )
{
    char* cb; /* char part buffer for strtod() */

    /* dictionary */
    char* rad = "radius";
    char* xcen = "x_center";
    char* ycen = "y_center";

    if ( !strcmp( buf, rad ) ) {
        parse_next( parser, event );
        coil->radius[( *map_seq ) - 1] =
            strtod( (char*)event->data.scalar.value, &cb );
    } else if ( !strcmp( buf, xcen ) ) {
        parse_next( parser, event );
        coil->x_center[( *map_seq ) - 1] =
            strtod( (char*)event->data.scalar.value, &cb );
    } else if ( !strcmp( buf, ycen ) ) {
        parse_next( parser, event );
        coil->y_center[( *map_seq ) - 1] =
            strtod( (char*)event->data.scalar.value, &cb );
    } else {
        printf( "\n -ERROR: Unknow variable in config file: %s\n", buf );
        exit( EXIT_FAILURE );
    }
}

void parse_next( yaml_parser_t* parser, yaml_event_t* event )
{
    /* parse next scalar. if wrong exit with error */
    if ( !yaml_parser_parse( parser, event ) ) {
        printf( "Parser error %d\n", parser->error );
        exit( EXIT_FAILURE );
    }
}

void init_prs( FILE** fh, yaml_parser_t* parser )
{
    /* parser initilization */
    if ( !yaml_parser_initialize( parser ) ) {
        fputs( "Failed to initialize parser!\n", stderr );
    }
    if ( *fh == NULL ) {
        fputs( "Failed to open file!\n", stderr );
    }

    yaml_parser_set_input_file( parser, *fh ); /* set input file */
}

void clean_prs( FILE** fh, yaml_parser_t* parser, yaml_event_t* event )
{
    yaml_event_delete( event );   /* delete event */
    yaml_parser_delete( parser ); /* delete parser */
    fclose( *fh );                /* close file */
}

void print_data( unsigned int nlps, Coil_t* coil )
{
    int i; /* counter */
    puts( "\n --- data structure after parsing ---" );
    printf( " current = %d\n", coil->cur );
    printf( " freq = %f\n", coil->freq );
    puts( " coil loops:" );
        puts( "\t -----------------" );
    for ( i = 0; i < nlps; i++ ) {
        printf( "\t radius = %.2f\n", coil->radius[i] );
        printf( "\t x_center = %.2f\n", coil->x_center[i] );
        printf( "\t y_center = %.2f\n", coil->y_center[i] );
        puts( "\t -----------------" );
    }
}

/* end */
