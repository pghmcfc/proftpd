/*
 * ProFTPD - FTP server daemon
 * Copyright (c) 1997, 1998 Public Flood Software
 * Copyright (C) 1999, 2000 MacGyver aka Habeeb J. Dihu <macgyver@tos.net>
 * Copyright (c) 2001, 2002 The ProFTPD Project team
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * As a special exemption, Public Flood Software/MacGyver aka Habeeb J. Dihu
 * and other respective copyright holders give permission to link this program
 * with OpenSSL, and distribute the resulting executable, without including
 * the source code for OpenSSL in the source distribution.
 */

/*
 * Configuration structure, server, command and associated prototypes.
 *
 * $Id: dirtree.h,v 1.20 2002-09-13 22:51:12 castaglia Exp $
 */

#ifndef __DIRTREE_H
#define __DIRTREE_H

#include "pool.h"
#include "sets.h"

typedef struct config_struc config_rec;
typedef struct privdata privdata_t;

struct conn_struc;

typedef struct server_struc {
  struct server_struc *next,*prev;

  pool *pool;			/* Memory pool for this server */
  xaset_t *set;			/* Set holding all servers */
  char *ServerName;		/* This server name */
  char *ServerAddress;		/* This server address */
  char *ServerFQDN;		/* Fully Qualified Domain Name */

  int ServerPort;		/* Port # to run on */
  int tcp_rwin,tcp_swin;	/* Receive/Send windows */
  int tcp_rwin_override;	/* Specifically override tcp rwin */
  int tcp_swin_override;	/* Specifically override tcp swin */

  char *ServerAdmin;		/* Administrator's name */
  char *ServerMessage;		/* Server's welcome message */

  int AnonymousGreeting;	/* Do not greet until after user login */  

  p_in_addr_t *ipaddr;		/* Internal address of this server */
  struct conn_struc *listen;	/* Our listening connection */
  xaset_t *conf;		/* Configuration details */

  int config_type;
} server_rec;

#define CLASS_USER

typedef struct cmd_struc {
  pool *pool;
  server_rec *server;
  config_rec *config;
  pool *tmp_pool;		/* Temporary pool which only exists
				 * while the cmd's handler is running
				 */
  int argc;

  char *arg;			/* entire argument (excluding command) */
  char **argv;
  char *group;			/* Command grouping */

  int  class;			/* The command class */
  int  symtable_index;		/* hack to speed up symbol hashing in modules.c */
  privdata_t *private;		/* Private data for passing/retaining between handlers */
  array_header *privarr;	/* Internal use */
} cmd_rec;

struct config_struc {
  struct config_struc *next,*prev;

  int config_type;
  pool *pool;			/* memory pool for this object */
  xaset_t *set;			/* The set we are stored in */
  char *name;
  int argc;
  void **argv;
  long overrides;		/* Override classes */
  long flags;			/* Flags */
  
  server_rec *server;		/* Server this config element is attached to */
  config_rec *parent;		/* Our parent configuration record */
  xaset_t *subset;		/* Sub-configuration */
};

#define CONF_ROOT		(1 << 0) /* No conf record */
#define CONF_DIR		(1 << 1) /* Per-Dir configuration */
#define CONF_ANON		(1 << 2) /* Anon. FTP configuration */
#define CONF_LIMIT		(1 << 3) /* Limits commands available */
#define CONF_VIRTUAL		(1 << 4) /* Virtual host */
#define CONF_DYNDIR		(1 << 5) /* .ftpaccess file */
#define CONF_GLOBAL		(1 << 6) /* "Global" context (applies to main server and ALL virtualhosts */
#define CONF_USERDATA		(1 << 14) /* Runtime user data */
#define CONF_PARAM		(1 << 15) /* config/args pair */

/* config_rec flags */
#define CF_MERGEDOWN		(1 << 0) /* Merge option down */
#define CF_MERGEDOWN_MULTI	(1 << 1) /* Merge down, allowing multiple instances */
#define CF_DYNAMIC		(1 << 2) /* Dynamically added entry */
#define CF_DEFER		(1 << 3) /* Defer hashing until authentication */

/* Operation codes for dir_* funcs */
#define OP_HIDE			1	/* Op for hiding dirs/files */
#define OP_COMMAND		2	/* Command operation */

/* For the Order directive */
#define ORDER_ALLOWDENY		0
#define ORDER_DENYALLOW		1

/* The following macro determines the "highest" level available for
 * configuration directives.  If a current dir_config is available, it's
 * subset is used, otherwise anon config or main server
 */

#define CURRENT_CONF		(session.dir_config ? session.dir_config->subset \
				 : (session.anon_config ? session.anon_config->subset \
                                    : main_server->conf))
#define TOPLEVEL_CONF		(session.anon_config ? session.anon_config->subset : main_server->conf)

extern server_rec		*main_server;
extern int			tcpBackLog;
extern int			SocketBindTight;
extern char			ServerType;
extern int			ServerMaxInstances;
extern int			ServerUseReverseDNS;
extern int			TimeoutLogin;
extern int			TimeoutIdle;
extern int			TimeoutNoXfer;
extern int			TimeoutStalled;

/* These macros are used to help handle configuration in modules */
#define CONF_ERROR(x, s)	return ERROR_MSG((x),NULL,pstrcat((x)->tmp_pool, \
				(x)->argv[0],": ",(s),NULL));

#define CHECK_ARGS(x, n)	if((x)->argc-1 < n) \
				CONF_ERROR(x,"missing arguments")

#define CHECK_VARARGS(x, n, m)	if((x)->argc - 1 < n || (x)->argc - 1 > m) \
				CONF_ERROR(x,"missing arguments")

#define CHECK_HASARGS(x, n)	((x)->argc - 1) == n

#define CHECK_CONF(x,p)		if(!check_conf((x),(p))) \
				CONF_ERROR((x), \
				pstrcat((x)->tmp_pool,"directive not allowed in ", \
				get_context_name((x)), \
				" context",NULL))

#define CHECK_CMD_ARGS(x, n)	if((x)->argc != (n)) { \
				  add_response_err(R_501, \
					"Invalid number of arguments."); \
				  return ERROR((x)); \
				}

#define CHECK_CMD_MIN_ARGS(x, n)	if((x)->argc < (n)) { \
					  add_response_err(R_501, \
					     "Invalid number of arguments."); \
					  return ERROR((x)); \
					}

/* Prototypes */

/* KLUDGE: disable umask() for not G_WRITE operations.  Config/
 * Directory walking code will be completely redesigned in 1.3,
 * this is only necessary for perfomance reasons in 1.1/1.2
 */
void kludge_disable_umask(void);
void kludge_enable_umask(void);

int define_exists(const char *);
void init_config(void);
void fixup_servers(void);
void set_config_stream(FILE *, unsigned int);
char *get_config_line(char *, size_t);
int parse_config_file(const char *);
config_rec *add_config_set(xaset_t **, const char *);
config_rec *add_config(const char *);
config_rec *add_config_param(const char *, int, ...);
config_rec *add_config_param_str(const char *, int, ...);
config_rec *add_config_param_set(xaset_t **, const char *, int, ...);
config_rec *find_config_next(config_rec *, config_rec *, int,
  const char *, int);
config_rec *find_config(xaset_t *, int, const char *, int);
void find_config_set_top(config_rec *);
int remove_config(xaset_t *, const char *, int);
class_t *find_class(p_in_addr_t *, char *);

array_header *parse_group_expression(pool *, int *, char **);
array_header *parse_user_expression(pool *, int *, char **);
int group_expression(char **);
int user_expression(char **);

long get_param_int(xaset_t *, const char *, int);
long get_param_int_next(const char *, int);
void *get_param_ptr(xaset_t *, const char *, int);
void *get_param_ptr_next(const char *, int);
void init_conf_stacks(void);
void free_conf_stacks(void);
server_rec *start_new_server(const char *);
server_rec *end_new_server(void);
config_rec *start_sub_config(const char *);
config_rec *end_sub_config(void);
char *get_word(char **);
char *get_line(char *, int, FILE *, unsigned int *);

config_rec *dir_match_path(pool *, char *);
void build_dyn_config(pool *, char *, struct stat *, int);
unsigned char dir_hide_file(const char *);
int dir_check_op_mode(pool *, char *, int, uid_t, gid_t, mode_t);
int dir_check_full(pool *, char *, char *, char *, int *);
int dir_check(pool *, char *, char *, char *, int *);
int dir_check_canon(pool *, char *, char *, char *, int *);
int is_dotdir(const char *);
int login_check_limits(xaset_t *, int, int, int *);
void resolve_anonymous_dirs(xaset_t *);
void resolve_defered_dirs(server_rec *);
void fixup_dirs(server_rec *, int);
unsigned char check_conf(cmd_rec *, int);
char *get_context_name(cmd_rec *);
int get_boolean(cmd_rec *, int);
char *get_full_cmd(cmd_rec *);
int match_ip(p_in_addr_t *, char *, const char *);

#endif /* __DIRTREE_H */
