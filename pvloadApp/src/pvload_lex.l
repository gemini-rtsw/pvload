%{
/*+*********************************************************************
  Module:       pvload_lex.{l,c}

  Author:       W. Lupton

  Description:  lex definitions for pv (EPICS process variable) loader
  *********************************************************************/
%}

/* Character class definitions */
digits   [0-9]
xdigits  [0-9a-fA-F]
exponent [Ee][-+]?{digits}+
pvname   [a-zA-Z0-9_:\.\(\)\$]
string   [a-zA-Z0-9_\,\./\*#\[\]%: ;!|\'\-&\(\)@\?\+<>=\$\\]

/* Start of rules */
%%

group	{ return( tokenGROUP ); }

sleep	{ return( tokenSLEEP ); }

arcsecs	|
arcsec	{ return( tokenARCSECS ); }

degrees	|
deg	{ return( tokenDEGREES ); }

microns |
um	{ return( tokenUM ); }

millimeters |
millimetres |
mm	{ return( tokenMM ); }

meters |
metres |
m	{ return( tokenM ); }

-?{digits}+ {
	sscanf( (const char *) yytext, "%d", &yylval.Int );
	return( tokenINTEGER );
	}

-?0[xX]{xdigits}+ {
	sscanf(  (const char *) yytext, "%x", &yylval.Int );
	return( tokenINTEGER );
	}

-?{digits}+"."{digits}*({exponent})?	|
-?{digits}*"."{digits}+({exponent})?	|
-?{digits}+{exponent} {
	sscanf(  (const char *) yytext, "%lf", &yylval.Real );
	return( tokenREAL );
	}

string	|
int	|
short	|
float	|
enum	|
char	|
long	|
double {
        yylval.Str = ( char * ) calloc( 1, strlen(  (const char *) yytext ) + 1 );
        strcpy( yylval.Str, (const char *) yytext );
        return( tokenTYPE );
    }

{pvname}+ {
        yylval.Str = ( char * ) calloc( 1, strlen(  (const char *) yytext ) + 1 );
        strcpy( yylval.Str,  (const char *) yytext );
        return( tokenPVNAME );
    }

\"{string}*\" {
        yylval.Str = ( char * ) calloc( 1, strlen(  (const char *) yytext ) + 1 );
        /* make sure that neither double quote gets passed back */
        strcpy( yylval.Str,  (const char *) yytext + 1 );
        yylval.Str[strlen( yylval.Str ) - 1] = '\0';

        return( tokenSTRING );
    }

"%"     { return( yytext[0] ); }
"{"     { return( yytext[0] ); }
"}"     { return( yytext[0] ); }
"["     { return( yytext[0] ); }
"]"     { return( yytext[0] ); }
"="     { return( yytext[0] ); }
"*"     { return( yytext[0] ); }
"/"     { return( yytext[0] ); }
","	{ return( yytext[0] ); }
";"	{ return( yytext[0] ); }

[ \t\r]*#.* { pvloadPrint( (char *) yytext ); }

[ \t\r]*\*.* {
	char *buff, *star;
	buff = ( char * ) calloc( 1, strlen( (const char *) yytext  + 1 ));
	strcpy( buff, (const char *)yytext );
	star = strchr( buff, '*' );
	if ( star != NULL ) *star = ' '; /* should never be NULL! */
	pvloadPrint( buff );
	free( buff );
    }

^[ \t\r]* { pvloadPrint((char *) yytext ); }

[ \t\r]* { /* ignore */ }

\n	{ pvloadPrint((char *) yytext ); line_num++; }

.	{ yyerror( "invalid character" ); }

%%
