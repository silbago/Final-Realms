/**
 * @main
 * List of the Handlers around on Final Realms
 *
 * Intention of list is to have a controlled list with updated
 * information. Makes it easy in general to scan mudlib for what
 * uses what. Command and control :)
 *
 * If you cause a typo/bug in this file you will crash the mud for sure.
 *
 * In the year of 2010 I started, mid 2011 will finish.
 *   1) Moving all handlers to /handlers/
 *   2) QC'ing all handlers being moved.
 *   3) Tables needs to be strengthened (semi-handlers).
 *   4) Interdependency enhancing and diagnostics enhancing.
 *
 * @author Silbago
 */

/** @ignore start */
#ifndef __INCLUDE_HANDLERS_H__
#define __INCLUDE_HANDLERS_H__
/** @ignore end */

#define HAND_ROOT      "/obj/handlers/"
#define COMMAND_TABLE  "/table/command_table"
#define OPTIONS_TABLE  "/table/options"
#define COMMAND_PARSER "/obj/handlers/parsers/command_parse"

/**
 * Login control, banishement, suspensions and site denial setup
 */
#define BASTARDS "/secure/bastards"

// Secure handlers and misc handlers outside main handler directory
//
#define OMIQ_SCHEDULES              "/handlers/omiq/schedules"
#define OMIQ_HUB                    "/handlers/omiq/hub"

#define TELL_CACHE                  "/secure/handlers/tell"
#define SHIPYARD_HANDLER            "/secure/handlers/shipyard"
#define WALKER_SAVE_HANDLER         "/secure/user/walker"
#define CHANNELS_PARSER             "/cmds/handler/channels"
#define __CMD_HANDLER__             "/cmds/handler/cmd_handler"
#define SPELLS_HANDLER              "/table/spells"
#define PLAYER_HOUSE_CONNECTION     "/d/aprior/housing/connection"
#define BUILD_HOUSE                 "/d/aprior/housing/build"
#define BUY_HOUSE                   "/d/aprior/housing/buy"

#define FILE_ACCESS_HANDLER         "/secure/file_access"
#define FILE_ACCESS_HUB             "/secure/file_hub"
#define FILE_ACCESS_LOAD            "/secure/file_load"

#define ACCOUNT_HANDLER             "/secure/handlers/account/account"
#define ACCOUNT_TRACKER             "/secure/handlers/account/tracker"
#define ACCOUNT_CLUSTER             "/secure/handlers/account/cluster"
#define ACCOUNT_CLEANUP             "/secure/handlers/account/cleanup"
#define ACCOUNT_LOGIN               "/secure/handlers/account/login"
#define ACCOUNT_HASH                "/secure/handlers/account/hash"

// Handlers concerning group interaction
//
#define GROUPS_HANDLER              "/secure/usergroups"
#define GROUPS_SYNC_HANDLER         "/secure/handlers/group_sync"
#define COUNCIL_HANDLER             "/handlers/groups/council"
#define RENTAL_HANDLER              "/handlers/groups/rental"
#define RACE_GROUP_HANDLER          "/handlers/groups/race_group"
#define INFLATION_HANDLER           "/handlers/groups/inflation"
#define RACES_HANDLER               "/handlers/groups/races"
#define ALIGNMENT_GROUPS_HANDLER    "/handlers/groups/alignment"
#define SCHOOLS_HANDLER             "/handlers/groups/schools"
#define NEWBIES_HANDLER             "/handlers/groups/newbies"
#ifndef CRIMINAL_HANDLER
#define CRIMINAL_HANDLER            "/obj/handlers/groups/criminal"
#endif
#define GUILD_LIST_HANDLER          "/obj/handlers/groups/guilds"
#define GUILD_TITLE_TABLE           "/table/guild_title_table"
#define GROUP_TITLE_HANDLER         "/handlers/groups/titles"

// Misc handlers
//
#ifdef COMBAT_HANDLER
#undef COMBAT_HANDLER
#endif
#define COMBAT_HANDLER              "/handlers/combat"
#define FEATS_HANDLER               "/handlers/feats"
#define FILECHANGE_HANDLER          "/handlers/filechange"
#define DEATH_HANDLER               "/handlers/death"
#define ASSASSIN_HANDLER            "/handlers/assassin"

#define GOD_HANDLER                 "/obj/handlers/god_handler"
#define SOUL_HANDLER                "/obj/handlers/soul"
#define FACTION_HANDLER             "/handlers/groups/faction"
#define TERM_HANDLER                "/obj/handlers/term"
#define ID_GENERATOR                "/handlers/id"
#define ENVIRONMENT_HANDLER         "/handlers/room/env_effects"
#define MONEY_HANDLER               "/handlers/money"
#define OPTIONS_HANDLER             "/handlers/options"
#define RANGE_HANDLER               "/obj/handlers/range"
#define TERMINAL_HANDLER            "/obj/handlers/term"
#define LANGUAGE_HANDLER            "/handlers/languages"
#define LOGIN_HANDLER               "/handlers/login"
#define STARGATE_HANDLER            "/obj/handlers/stargate"
#define VIRTUAL_ROOM                "/obj/handlers/virtual_room/vroom_handler.c"
#define STATUES_HANDLER             "/handlers/statues"

// Dealing with climate, weather, timeline, coordinates.
//
#define WEATHER_HANDLER             "/handlers/room/weather"
#define TIMELINE_HANDLER            "/handlers/room/timeline"
#define COORDINATES_HANDLER         "/handlers/room/coordinates"
#define WAYPOINTS_HANDLER           "/handlers/room/waypoints"

// Quest and Mission handlers and specialised subhandlers
//
#define QUESTS_HANDLER              "/obj/handlers/quests"
#define QUEST_INDEX                 "/obj/handlers/quests/index"
#define QUEST_REWARD                "/obj/handlers/quests/reward"
#define QUEST_MODIFY                "/obj/handlers/quests/modify"
#define QUEST_WALKTHROUGH           "/secure/user/walkthrough"
#define QUEST_REPORT                "/obj/handlers/quests/report"
#define MISSIONS_HANDLER            "/obj/handlers/quests/missions"

// Task management, interdependent databases for global queries and
// control. Quality controls and certificates. Notebook and errors.
//
#define ERRORS_HANDLER              "/obj/handlers/doc/errors"
#define ERR_ARCHIVE                 "/obj/handlers/doc/err_archive"
#define ALIGNMENT_HANDLER           "/obj/handlers/doc/alignment"
#define CERTIFICATES_HANDLER        "/handlers/managers/certificates"

// Help; Database indexes, helpfile writing and parsing.
//
#define CMDS_HELP                   "/handlers/help/index_cmds"
#define SOULS_HELP                  "/handlers/help/index_souls"
#define MAN_LOCAL_HELP              "/handlers/help/index_man_local"
#define MAN_DRIVER_HELP             "/handlers/help/index_man_driver"
#define MAN_DRIVER_HTML_HELP        "/handlers/help/index_man_driver_html"
#define MORTAL_HELP                 "/handlers/help/index_mortal"
#define PLAYERS_HELP                "/handlers/help/index_players"
#define AUTODOC_INDEXER             "/handlers/help/index_autodoc"
#define CREATORS_HELP               "/handlers/help/index_creators"

#define HELPDOC_GODS                "/handlers/help/generate_gods"
#define HELPDOC_GUILDS              "/handlers/help/generate_guilds"
#define HELPDOC_OMIQ                "/handlers/help/generate_omiq"
#define HELPDOC_RACEGUILDS          "/handlers/help/generate_raceguilds"
#define HELPDOC_SPELL_TABLE         "/handlers/help/generate_spell_table"
#define GENERATE_CMDS               "/handlers/help/generate_cmds"

#define AUTODOC_HANDLER             "/handlers/help/autodoc"
#define AUTODOC_QUEUE_HANDLER       "/handlers/help/autodoc_queue"
#define AUTODOC_NROFF_HANDLER       "/handlers/help/autodoc_nroff"
#define HELP_NROFF_HANDLER          "/handlers/help/help_nroff"
#define NROFF_HANDLER               "/handlers/help/nroff"

// Mail Handlers
//
#define MAIL_ADDRESS_PARSER         "/secure/handlers/mail/address_parser"
#define MAIL_FILTER                 "/secure/handlers/mail/mail_filter"
#define POST_OFFICE                 "/secure/handlers/mail/post_office"
#define MAIL_GATEWAY                "/secure/handlers/mail/gateway"
#define MAIL_BOX                    "/secure/handlers/mail/mail_box"
#define SMTP_DAEMON                 "/secure/handlers/mail/smtp"
#define POP3_DAEMON                 "/secure/handlers/mail/pop3"
#define MIGRATE                     "/secure/handlers/mail/migrate"

// Previous version mail systems, is to be deleted later
// Once the new system gets the "old interface"
// Also need to dig through these and track down what use it
// Is intermud/pop3/smtp implemented by some way too ?
//
#define MAIL_HANDLER                "/obj/handlers/messagers/mail"
#define MAIL_SERVER                 "/obj/handlers/messagers/mail_server"
#define MAIL_TRACKER                "/obj/handlers/messagers/mail_tracker"
#define POST_HANDLER                "/obj/handlers/messagers/post"
#define POST_READER                 "/obj/handlers/messagers/post_reader"
#define POSTAL_DAEMON               "/obj/handlers/messagers/postal_daemon"
#define MAIL_BOX_HANDLER            "/obj/handlers/messagers/mail_box"
#define MAIL_LETTER_HANDLER         "/obj/handlers/messagers/mail_letters"
#define MAIL_GROUPS_HANDLER         "/obj/handlers/messagers/mail_groups"
#define LETTER_DAEMON               "/obj/handlers/messagers/letter_daemon"

// Communication handlers
//
#define CHANNELS_HANDLER            "/obj/handlers/messagers/channels"
#define BOARD_HANDLER               "/obj/handlers/messagers/board"
#define BLOCK_HANDLER               "/obj/handlers/messagers/block"
#define SHOUT_CURSE_FILTER          "/obj/handlers/messagers/shoutfilter"

// Object trackers, and other statistical engines
//
#define SPELLS_INFO_HANDLER         "/handlers/info/spells"
#define GUILD_INFO_HANDLER          "/handlers/info/guilds"
#define ITEM_INFO_HANDLER           "/handlers/info/item_info"
#define NPC_INFO_HANDLER            "/handlers/info/npc_info"
#define OBJECT_TRACK_HANDLER        "/obj/handlers/trackers/object_track"
#define IOU_HANDLER                 "/handlers/trackers/ious.c"

#define PLAYER_STATISTICS_HANDLER   "/secure/misc/diagnostics/stats"
#define PK_STAT_HANDLER             "/obj/handlers/trackers/pk_stat"
#define PK_HANDLER                  "/obj/handlers/trackers/pk"

#define DEATH_TRACKER               "/handlers/trackers/death"
#define MONEY_INFO_HANDLER          "/obj/handlers/trackers/money_info"
#define MONEY_TRACKER_HANDLER       "/handlers/trackers/money"
#define STAT_TRACKER_HANDLER        "/obj/handlers/trackers/stat_tracker"
#define NEWBIE_DEATH_HANDLER        "/obj/handlers/trackers/newbie_death"
#define ALIGN_TRACKER_HANDLER       "/obj/handlers/trackers/align_tracker"
#define LEVEL_TRACKER_HANDLER       "/obj/handlers/trackers/level_tracker"

// Ranking list handlers, obscure
//
#define ACHIEVEMENTS_LEADERBOARD    "/handlers/leaderboard/achievements"
#define CATHEDRAL_LEADERBOARD       "/handlers/leaderboard/cathedral"
#define RESURRECT_LEADERBOARD       "/handlers/leaderboard/resurrect"
#define NEWBIE_PK_LEADERBOARD       "/handlers/leaderboard/newbie_pk"
#define AGE_IMM_LEADERBOARD         "/handlers/leaderboard/age_imm"
#define QUESTS2_LEADERBOARD         "/handlers/leaderboard/quests2"
#define QUESTS_LEADERBOARD          "/handlers/leaderboard/quests"
#define SKILLS_LEADERBOARD          "/handlers/leaderboard/skills"
#define ACTIVE_LEADERBOARD          "/handlers/leaderboard/active"
#define LEVEL_LEADERBOARD           "/handlers/leaderboard/level"
#define MONEY_LEADERBOARD           "/handlers/leaderboard/money"
#define AGE_LEADERBOARD             "/handlers/leaderboard/age"
#define TMC_LEADERBOARD             "/handlers/leaderboard/tmc"
#define XP_LEADERBOARD              "/handlers/leaderboard/xp"

#define PK_LEADERBOARD              "/handlers/leaderboard/pk"
#define PK_SIG_LEADERBOARD          "/handlers/leaderboard/pk_sig"
#define PK_RACE_LEADERBOARD         "/handlers/leaderboard/pk_race"
#define PK_GUILD_LEADERBOARD        "/handlers/leaderboard/pk_guild"
#define PK_RACEGROUP_LEADERBOARD    "/handlers/leaderboard/pk_racegroup"

// Protocols
//
#define GMCP_HANDLER                "/handlers/protocols/gmcp"

#endif
