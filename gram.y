/*
 *   $Id: gram.y,v 1.1.1.1 1997/10/14 17:17:40 lf Exp $
 *
 *   Authors:
 *    Pedro Roque		<roque@di.fc.ul.pt>
 *    Lars Fenneberg		<lf@elemental.net>	 
 *
 *   This software is Copyright 1996 by the above mentioned author(s), 
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <lf@elemental.net>.
 *
 */
%{
#include <config.h>
#include <includes.h>
#include <radvd.h>
#include <defaults.h>

extern struct Interface *IfaceList;
struct Interface *iface = NULL;
struct AdvPrefix *prefix = NULL;

extern char *conf_file;
extern int num_lines;
extern char *yytext;
extern int sock;

static int palloc_check(void);

static void cleanup(void);
static void yyerror(char *msg);

#define ABORT	do { cleanup(); YYABORT; } while (0);

%}

%token		T_INTERFACE
%token		T_PREFIX

%token	<str>	STRING
%token	<num>	NUMBER
%token	<bool>	SWITCH
%token	<addr>	IPV6ADDR
%token 		INFINITY

%token		T_AdvSendAdvert
%token		T_MaxRtrAdvInterval
%token		T_MinRtrAdvInterval
%token		T_AdvManagedFlag
%token		T_AdvOtherConfigFlag
%token		T_AdvLinkMTU
%token		T_AdvReachableTime
%token		T_AdvRetransTimer
%token		T_AdvCurHopLimit
%token		T_AdvDefaultLifetime
%token		T_AdvSourceLLAddress

%token		T_AdvOnLink
%token		T_AdvAutonomous
%token		T_AdvValidLifetime
%token		T_AdvPreferredLifetime

%token		T_BAD_TOKEN

%type	<str>	name
%type	<pinfo> prefixdef prefixlist
%type   <num>	number_or_infinity

%union {
	int			num;
	int			bool;
	struct in6_addr		*addr;
	char			*str;
	struct AdvPrefix	*pinfo;
};

%%

grammar		: grammar ifacedef
		| ifacedef
		;

ifacedef	: T_INTERFACE name '{' ifaceparams  '}' ';'
		{
			struct Interface *iface2;

			strcpy(iface->Name, $2);

			iface2 = IfaceList;
			while (iface2)
			{
				if (!strcmp(iface2->Name, iface->Name))
				{
					log(LOG_ERR, "duplicate interface "
						"definition for %s", iface->Name);

					ABORT;
				}
				iface2 = iface2->next;
			}			

			if (check_device(sock, iface) < 0)
				ABORT;
			if (setup_deviceinfo(sock, iface) < 0)
				ABORT;
			if (check_iface(iface) < 0)
				ABORT;
			if (setup_linklocal_addr(sock, iface) < 0)
				ABORT;

			iface->next = IfaceList;
			IfaceList = iface;

			dlog(LOG_DEBUG, 4, "interface definition for %s is ok", iface->Name);

			iface = NULL;
		};
	
name		: STRING
		{
			/* check vality */
			$$ = $1;
		}
		;

ifaceparams	: iface_advt optional_ifacevlist prefixlist
		{
			iface->AdvPrefixList = $3;
		}
		;

optional_ifacevlist: /* empty */
		   | ifacevlist
		   ;

ifacevlist	: ifacevlist ifaceval
		| ifaceval
		;

iface_advt	: T_AdvSendAdvert SWITCH ';'
		{
			iface = malloc(sizeof(struct Interface));

			if (iface == NULL) {
				log(LOG_CRIT, "malloc failed: %s", strerror(errno));
				ABORT;
			}

			iface_init_defaults(iface);
			iface->AdvSendAdvert = $2;
		}

ifaceval	: T_MinRtrAdvInterval NUMBER ';'
		{
			iface->MinRtrAdvInterval = $2;
		}
		| T_MaxRtrAdvInterval NUMBER ';'
		{
			iface->MaxRtrAdvInterval = $2;
		}
		| T_AdvManagedFlag SWITCH ';'
		{
			iface->AdvManagedFlag = $2;
		}
		| T_AdvOtherConfigFlag SWITCH ';'
		{
			iface->AdvOtherConfigFlag = $2;
		}
		| T_AdvLinkMTU NUMBER ';'
		{
			iface->AdvLinkMTU = $2;
		}
		| T_AdvReachableTime NUMBER ';'
		{
			iface->AdvReachableTime = $2;
		}
		| T_AdvRetransTimer NUMBER ';'
		{
			iface->AdvRetransTimer = $2;
		}
		| T_AdvDefaultLifetime NUMBER ';'
		{
			iface->AdvDefaultLifetime = $2;
		}
		| T_AdvCurHopLimit NUMBER ';'
		{
			iface->AdvCurHopLimit = $2;
		}
		| T_AdvSourceLLAddress SWITCH ';'
		{
			iface->AdvSourceLLAddress = $2;
		}
		;
		
prefixlist	: prefixdef
		{
			$$ = $1;
		}
		| prefixlist prefixdef
		{
			$2->next = $1;
			$$ = $2;
		}
		;

prefixdef	: T_PREFIX IPV6ADDR '/' NUMBER '{' optional_prefixplist '}' ';'
		{
			if (palloc_check() < 0)
				ABORT;

			if ($4 > MAX_PrefixLen)
			{
				log(LOG_ERR, "invalid prefix length in %s, line %d", conf_file, num_lines);
				ABORT;
			}
			

			prefix->PrefixLen = $4;

			if (prefix->AdvPreferredLifetime >
			    prefix->AdvValidLifetime)
			{
				log(LOG_ERR, "AdvValidLifeTime must be "
					"greater than AdvPreferredLifetime in %s, line %d", 
					conf_file, num_lines);
				ABORT;
			}

			memcpy(&prefix->Prefix, $2, sizeof(struct in6_addr));
			
			$$ = prefix;
			prefix = NULL;
		}
		;

optional_prefixplist: /* empty */
		    | prefixplist 

prefixplist	: prefixplist prefixparms
		| prefixparms
		;

prefixparms	: T_AdvOnLink SWITCH ';'
		{
			if (palloc_check() < 0)
				ABORT;

			prefix->AdvOnLinkFlag = $2;
		}
		| T_AdvAutonomous SWITCH ';'
		{
			if (palloc_check() < 0)
				ABORT;

			prefix->AdvAutonomousFlag = $2;
		}
		| T_AdvValidLifetime number_or_infinity ';'
		{
			if (palloc_check() < 0)
				ABORT;

			prefix->AdvValidLifetime = $2;
		}
		| T_AdvPreferredLifetime number_or_infinity ';'
		{
			if (palloc_check() < 0)
				ABORT;

			prefix->AdvPreferredLifetime = $2;
		}
		;

number_or_infinity      : NUMBER
                        {
                                $$ = $1; 
                        }
                        | INFINITY
                        {
                                $$ = (u_int32_t)~0;
                        }
                        ;

%%

static
void cleanup(void)
{
	if (iface)
		free(iface);
	
	if (prefix)
		free(prefix);
}

static void
yyerror(char *msg)
{
	cleanup();
	log(LOG_ERR, "%s in %s, line %d: %s", msg, conf_file, num_lines, yytext);
}

static int
palloc_check(void)
{
	if (prefix == NULL)
	{
		prefix = malloc(sizeof(struct AdvPrefix));
		
		if (prefix == NULL) {
			log(LOG_CRIT, "malloc failed: %s", strerror(errno));
			return (-1);
		}

		prefix_init_defaults(prefix);
	}

	return 0;
}
