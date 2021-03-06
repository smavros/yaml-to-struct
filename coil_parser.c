/* Example code: yaml file parser using libyaml
 * author: smavros
 */

#include <stdbool.h>
#include <stdio.h>
#include <yaml.h>

/* Types  ------------------------------------------------------------- */

typedef struct coil {
    double freq;
    double* radius;
    double* x_center;
    double* y_center;
    unsigned int cur;
} coil_t;

/* Prototypes --------------------------------------------------------- */

/* Struct initialization */
void init_coil( coil_t* coil, char** argv );
void realloc_coil( unsigned int nlp, coil_t* coil );
void free_coil( coil_t* coil );

/* Global parser */
unsigned int parser( coil_t* coil, char** argv );

/* Parser utilities */
void init_prs( FILE* fp, yaml_parser_t* parser );
void parse_next( yaml_parser_t* parser, yaml_event_t* event );
void clean_prs( FILE* fp, yaml_parser_t* parser, yaml_event_t* event );

/* Parser actions */
void event_switch( bool* seq_status, unsigned int* map_seq, coil_t* coil,
                   yaml_parser_t* parser, yaml_event_t* event, FILE* fp );
void to_data( bool* seq_status, unsigned int* map_seq, coil_t* coil,
              yaml_parser_t* parser, yaml_event_t* event, FILE* fp );
void to_data_from_map( char* buf, unsigned int* map_seq, coil_t* coil,
                       yaml_parser_t* parser, yaml_event_t* event, FILE* fp );

/* Post parsing utilities */
void parsed_loops( unsigned int* nlp, char** argv, coil_t* coil );
void print_data( unsigned int nlps, coil_t* coil );

/* Main --------------------------------------------------------------- */

int
main( int argc, char** argv )
{
    if ( argc != 2 || atoi( argv[1] ) <= 0 ) {
        puts( " Arguments: give number of coil loops to be parsed" );
        return EXIT_FAILURE; 
    }

    coil_t coil; /* Create my coil */

    init_coil( &coil, argv ); /* Initialize my coil */

    unsigned int nlp;            /* Number of loops in config file */
    nlp = parser( &coil, argv ); /* Parse my coil */

    /* Check if requested coil loops number is consistent with config file */
    parsed_loops( &nlp, argv, &coil );

    print_data( nlp, &coil );

    free_coil( &coil );

    return EXIT_SUCCESS;
}

/* Definitions -------------------------------------------------------- */

void
parsed_loops( unsigned int* nlp, char** argv, coil_t* coil )
{
    if ( ( *nlp ) > (unsigned int)atoi( argv[1] ) ) {

        puts( "\n -INFO: The config file contains more coil loops than "
              "the number that was asked to be parsed in!" );
        *nlp = atoi( argv[1] );

    } else if ( ( *nlp ) < (unsigned int)atoi( argv[1] ) ) {
        /*  In case of larger number of arg coils compared to the
         *  configuration file denoted coils one must reallocate the coil
         *  in order to avoid overflow errors over data structure
         */
        puts( "\n -INFO: The config file contains fewer coil loops than "
              "the number that was asked to be parsed in!" );
        puts( " -INFO: Parsed data structure reallocated!" );

        realloc_coil( *nlp, coil );
    }
}

void
realloc_coil( unsigned int nlp, coil_t* coil )
{
    coil->radius = realloc( coil->radius, nlp * sizeof( double* ) );
    coil->x_center = realloc( coil->x_center, nlp * sizeof( double* ) );
    coil->y_center = realloc( coil->y_center, nlp * sizeof( double* ) );
}

void
init_coil( coil_t* coil, char** argv )
{
    coil->radius = calloc( atoi( argv[1] ), sizeof( double* ) );
    coil->x_center = calloc( atoi( argv[1] ), sizeof( double* ) );
    coil->y_center = calloc( atoi( argv[1] ), sizeof( double* ) );
}

void
free_coil( coil_t* coil )
{
    free( coil->radius );
    free( coil->x_center );
    free( coil->y_center );
}

unsigned int
parser( coil_t* coil, char** argv )
{
    /* Open file & declare libyaml types */
    FILE* fp = fopen( "data.yml", "r" );
    yaml_parser_t parser;
    yaml_event_t event;

    bool seq_status = 0;      /* IN or OUT of sequence index, init to OUT */
    unsigned int map_seq = 0; /* Index of mapping inside sequence */

    init_prs( fp, &parser );  /* Initiliaze parser & open file */

    do {
        
        parse_next( &parser, &event ); /* Parse new event */

        /* Decide what to do with each event */
        event_switch( &seq_status, &map_seq, coil, &parser, &event, fp );

        if ( event.type != YAML_STREAM_END_EVENT ) {
            yaml_event_delete( &event );
        }

        if ( map_seq > (unsigned int)atoi( argv[1] ) ) {
            break;
        }
    
    } while ( event.type != YAML_STREAM_END_EVENT );

    clean_prs( fp, &parser, &event ); /* clean parser & close file */

    return map_seq;
}

void
event_switch( bool* seq_status, unsigned int* map_seq, coil_t* coil,
              yaml_parser_t* parser, yaml_event_t* event, FILE* fp )
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
            to_data( seq_status, map_seq, coil, parser, event, fp );
            break;
        case YAML_NO_EVENT:
            puts( " ERROR: No event!" );
            exit( EXIT_FAILURE );
            break;
    }
}

void
to_data( bool* seq_status, unsigned int* map_seq, coil_t* coil,
         yaml_parser_t* parser, yaml_event_t* event, FILE* fp )
{
    char* buf = (char*)event->data.scalar.value;

    /* Dictionary */
    char* cur = "cur";
    char* freq = "freq";
    char* loops = "loops";

    if ( !strcmp( buf, cur ) ) {
        yaml_event_delete( event ); 
        parse_next( parser, event );
        coil->cur = atoi( (char*)event->data.scalar.value );
    } else if ( !strcmp( buf, freq ) ) {
        yaml_event_delete( event ); 
        parse_next( parser, event );
        coil->freq = strtod( (char*)event->data.scalar.value, NULL );
    } else if ( ( *seq_status ) == true ) {
        /* Data from sequence of loops */
        to_data_from_map( buf, map_seq, coil, parser, event, fp );
    } else if ( !strcmp( buf, loops ) ) {
        /* Do nothing, "loops" is just the label of mapping's sequence */
    } else {
        printf( "\n -ERROR: Unknow variable in config file: %s\n", buf );
        free_coil( coil );
        clean_prs( fp, parser, event ); 
        exit( EXIT_FAILURE );
    }
}

void
to_data_from_map( char* buf, unsigned int* map_seq, coil_t* coil,
                  yaml_parser_t* parser, yaml_event_t* event, FILE* fp )
{
    /* Dictionary */
    char* rad = "radius";
    char* xcen = "x_center";
    char* ycen = "y_center";

    if ( !strcmp( buf, rad ) ) {
        yaml_event_delete( event ); 
        parse_next( parser, event );
        coil->radius[( *map_seq ) - 1] =
            strtod( (char*)event->data.scalar.value, NULL );
    } else if ( !strcmp( buf, xcen ) ) {
        yaml_event_delete( event ); 
        parse_next( parser, event );
        coil->x_center[( *map_seq ) - 1] =
            strtod( (char*)event->data.scalar.value, NULL );
    } else if ( !strcmp( buf, ycen ) ) {
        yaml_event_delete( event ); 
        parse_next( parser, event );
        coil->y_center[( *map_seq ) - 1] =
            strtod( (char*)event->data.scalar.value, NULL );
    } else {
        printf( "\n -ERROR: Unknow variable in config file: %s\n", buf );
        free_coil( coil );
        clean_prs( fp, parser, event ); 
        exit( EXIT_FAILURE );
    }
}

void
parse_next( yaml_parser_t* parser, yaml_event_t* event )
{
    /* Parse next scalar. if wrong exit with error */
    if ( !yaml_parser_parse( parser, event ) ) {
        printf( "Parser error %d\n", parser->error );
        exit( EXIT_FAILURE );
    }
}

void
init_prs( FILE* fp, yaml_parser_t* parser )
{
    /* Parser initilization */
    if ( !yaml_parser_initialize( parser ) ) {
        fputs( "Failed to initialize parser!\n", stderr );
    }

    if ( fp == NULL ) {
        fputs( "Failed to open file!\n", stderr );
    }

    yaml_parser_set_input_file( parser, fp ); 
}

void
clean_prs( FILE* fp, yaml_parser_t* parser, yaml_event_t* event )
{
    yaml_event_delete( event );   /* Delete event */
    yaml_parser_delete( parser ); /* Delete parser */
    fclose( fp );                 /* Close file */
}

void
print_data( unsigned int nlps, coil_t* coil )
{
    puts( "\n --- data structure after parsing ---" );
    printf( " current = %d\n", coil->cur );
    printf( " freq = %f\n", coil->freq );

    puts( " coil loops:" );
    puts( "\t -----------------" );
    for ( int i = 0; i < (int)nlps; i++ ) {
        printf( "\t radius = %.2f\n", coil->radius[i] );
        printf( "\t x_center = %.2f\n", coil->x_center[i] );
        printf( "\t y_center = %.2f\n", coil->y_center[i] );
        puts( "\t -----------------" );
    }
}

/* End */
