/* -*- mode: c -*- */

/* Copyright (C) 2014-2017 Alexander Chernov <cher@ejudge.ru> */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ejudge/super_proto.h"

const unsigned char super_proto_is_http_request[] =
{
  [SSERV_CMD_PASS_FD] = 0,
  [SSERV_CMD_STOP] = 0,
  [SSERV_CMD_RESTART] = 0,
  [SSERV_CMD_OPEN_CONTEST] = 0,
  [SSERV_CMD_CLOSE_CONTEST] = 0,
  [SSERV_CMD_INVISIBLE_CONTEST] = 0,
  [SSERV_CMD_VISIBLE_CONTEST] = 0,
  [SSERV_CMD_SHOW_HIDDEN] = 0,
  [SSERV_CMD_HIDE_HIDDEN] = 0,
  [SSERV_CMD_SHOW_CLOSED] = 0,
  [SSERV_CMD_HIDE_CLOSED] = 0,
  [SSERV_CMD_SHOW_UNMNG] = 0,
  [SSERV_CMD_HIDE_UNMNG] = 0,
  [SSERV_CMD_RUN_LOG_TRUNC] = 0,
  [SSERV_CMD_RUN_LOG_DEV_NULL] = 0,
  [SSERV_CMD_RUN_LOG_FILE] = 0,
  [SSERV_CMD_RUN_MNG_TERM] = 0,
  [SSERV_CMD_CONTEST_RESTART] = 0,
  [SSERV_CMD_CLEAR_MESSAGES] = 0,
  [SSERV_CMD_RUN_MNG_RESET_ERROR] = 0,
  [SSERV_CMD_CNTS_FORGET] = 0,
  [SSERV_CMD_CNTS_DEFAULT_ACCESS] = 0,
  [SSERV_CMD_CNTS_ADD_RULE] = 0,
  [SSERV_CMD_CNTS_CHANGE_RULE] = 0,
  [SSERV_CMD_CNTS_DELETE_RULE] = 0,
  [SSERV_CMD_CNTS_UP_RULE] = 0,
  [SSERV_CMD_CNTS_DOWN_RULE] = 0,
  [SSERV_CMD_CNTS_COPY_ACCESS] = 0,
  [SSERV_CMD_CNTS_DELETE_PERMISSION] = 0,
  [SSERV_CMD_CNTS_ADD_PERMISSION] = 0,
  [SSERV_CMD_CNTS_SAVE_PERMISSIONS] = 0,
  [SSERV_CMD_CNTS_SET_PREDEF_PERMISSIONS] = 0, /* implemented in serve-control */
  [SSERV_CMD_CNTS_SAVE_FORM_FIELDS] = 0,
  [SSERV_CMD_CNTS_SAVE_CONTESTANT_FIELDS] = 0, /* NOTE: the following 5 commands must */
  [SSERV_CMD_CNTS_SAVE_RESERVE_FIELDS] = 0,    /* be sequental */
  [SSERV_CMD_CNTS_SAVE_COACH_FIELDS] = 0,
  [SSERV_CMD_CNTS_SAVE_ADVISOR_FIELDS] = 0,
  [SSERV_CMD_CNTS_SAVE_GUEST_FIELDS] = 0,
  [SSERV_CMD_LANG_SHOW_DETAILS] = 0,
  [SSERV_CMD_LANG_HIDE_DETAILS] = 0,
  [SSERV_CMD_LANG_DEACTIVATE] = 0,
  [SSERV_CMD_LANG_ACTIVATE] = 0,
  [SSERV_CMD_PROB_ADD_ABSTRACT] = 0,
  [SSERV_CMD_PROB_ADD] = 0,
  [SSERV_CMD_PROB_SHOW_DETAILS] = 0,
  [SSERV_CMD_PROB_HIDE_DETAILS] = 0,
  [SSERV_CMD_PROB_SHOW_ADVANCED] = 0,
  [SSERV_CMD_PROB_HIDE_ADVANCED] = 0,
  [SSERV_CMD_PROB_DELETE] = 0,
  [SSERV_CMD_PROB_CHANGE_VARIANTS] = 0,
  [SSERV_CMD_PROB_DELETE_VARIANTS] = 0,
  [SSERV_CMD_LANG_UPDATE_VERSIONS] = 0,
  [SSERV_CMD_PROB_CLEAR_VARIANTS] = 0,
  [SSERV_CMD_PROB_RANDOM_VARIANTS] = 0,
  [SSERV_CMD_LOGOUT] = 0,
  [SSERV_CMD_HTTP_REQUEST] = 0,

/* subcommands for SSERV_CMD_HTTP_REQUEST */
  [SSERV_CMD_EDITED_CNTS_BACK] = 1,
  [SSERV_CMD_EDITED_CNTS_CONTINUE] = 1,
  [SSERV_CMD_EDITED_CNTS_START_NEW] = 1,
  [SSERV_CMD_LOCKED_CNTS_FORGET] = 1,
  [SSERV_CMD_LOCKED_CNTS_CONTINUE] = 1,
  [SSERV_CMD_USER_FILTER_CHANGE_ACTION] = 1,
  [SSERV_CMD_USER_FILTER_FIRST_PAGE_ACTION] = 1,
  [SSERV_CMD_USER_FILTER_PREV_PAGE_ACTION] = 1,
  [SSERV_CMD_USER_FILTER_NEXT_PAGE_ACTION] = 1,
  [SSERV_CMD_USER_FILTER_LAST_PAGE_ACTION] = 1,
  [SSERV_CMD_USER_JUMP_CONTEST_ACTION] = 1,
  [SSERV_CMD_USER_JUMP_GROUP_ACTION] = 1,
  [SSERV_CMD_USER_BROWSE_MARK_ALL_ACTION] = 1,
  [SSERV_CMD_USER_BROWSE_UNMARK_ALL_ACTION] = 1,
  [SSERV_CMD_USER_BROWSE_TOGGLE_ALL_ACTION] = 1,
  [SSERV_CMD_USER_CREATE_ONE_PAGE] = 1,
  [SSERV_CMD_USER_CREATE_ONE_ACTION] = 1,
  [SSERV_CMD_USER_CREATE_MANY_PAGE] = 1,
  [SSERV_CMD_USER_CREATE_MANY_ACTION] = 1,
  [SSERV_CMD_USER_CREATE_FROM_CSV_PAGE] = 1,
  [SSERV_CMD_USER_CREATE_FROM_CSV_ACTION] = 1,
  [SSERV_CMD_USER_DETAIL_PAGE] = 1,
  [SSERV_CMD_USER_PASSWORD_PAGE] = 1,
  [SSERV_CMD_USER_CHANGE_PASSWORD_ACTION] = 1,
  [SSERV_CMD_USER_CNTS_PASSWORD_PAGE] = 1,
  [SSERV_CMD_USER_CHANGE_CNTS_PASSWORD_ACTION] = 1,
  [SSERV_CMD_USER_CLEAR_FIELD_ACTION] = 1,
  [SSERV_CMD_USER_CREATE_MEMBER_ACTION] = 1,
  [SSERV_CMD_USER_DELETE_MEMBER_PAGE] = 1,
  [SSERV_CMD_USER_DELETE_MEMBER_ACTION] = 1,
  [SSERV_CMD_USER_SAVE_AND_PREV_ACTION] = 1,
  [SSERV_CMD_USER_SAVE_ACTION] = 1,
  [SSERV_CMD_USER_SAVE_AND_NEXT_ACTION] = 1,
  [SSERV_CMD_USER_CANCEL_AND_PREV_ACTION] = 1,
  [SSERV_CMD_USER_CANCEL_ACTION] = 1,
  [SSERV_CMD_USER_CANCEL_AND_NEXT_ACTION] = 1,
  [SSERV_CMD_USER_CREATE_REG_PAGE] = 1,
  [SSERV_CMD_USER_CREATE_REG_ACTION] = 1,
  [SSERV_CMD_USER_EDIT_REG_PAGE] = 1,
  [SSERV_CMD_USER_EDIT_REG_ACTION] = 1,
  [SSERV_CMD_USER_DELETE_REG_PAGE] = 1,
  [SSERV_CMD_USER_DELETE_REG_ACTION] = 1,
  [SSERV_CMD_USER_DELETE_SESSION_ACTION] = 1,
  [SSERV_CMD_USER_DELETE_ALL_SESSIONS_ACTION] = 1,
  [SSERV_CMD_USER_SEL_RANDOM_PASSWD_PAGE] = 1,
  [SSERV_CMD_USER_SEL_RANDOM_PASSWD_ACTION] = 1,
  [SSERV_CMD_USER_SEL_CLEAR_CNTS_PASSWD_PAGE] = 1,
  [SSERV_CMD_USER_SEL_CLEAR_CNTS_PASSWD_ACTION] = 1,
  [SSERV_CMD_USER_SEL_RANDOM_CNTS_PASSWD_PAGE] = 1,
  [SSERV_CMD_USER_SEL_RANDOM_CNTS_PASSWD_ACTION] = 1,
  [SSERV_CMD_USER_SEL_CREATE_REG_PAGE] = 1,
  [SSERV_CMD_USER_SEL_CREATE_REG_ACTION] = 1,
  [SSERV_CMD_USER_SEL_CREATE_REG_AND_COPY_PAGE] = 1,
  [SSERV_CMD_USER_SEL_CREATE_REG_AND_COPY_ACTION] = 1,
  [SSERV_CMD_USER_SEL_DELETE_REG_PAGE] = 1,
  [SSERV_CMD_USER_SEL_DELETE_REG_ACTION] = 1,
  [SSERV_CMD_USER_SEL_CHANGE_REG_STATUS_PAGE] = 1,
  [SSERV_CMD_USER_SEL_CHANGE_REG_STATUS_ACTION] = 1,
  [SSERV_CMD_USER_SEL_CHANGE_REG_FLAGS_PAGE] = 1,
  [SSERV_CMD_USER_SEL_CHANGE_REG_FLAGS_ACTION] = 1,
  [SSERV_CMD_USER_SEL_CANCEL_ACTION] = 1,
  [SSERV_CMD_USER_SEL_VIEW_PASSWD_PAGE] = 1,
  [SSERV_CMD_USER_SEL_VIEW_CNTS_PASSWD_PAGE] = 1,
  [SSERV_CMD_USER_SEL_VIEW_PASSWD_REDIRECT] = 1,
  [SSERV_CMD_USER_SEL_VIEW_CNTS_PASSWD_REDIRECT] = 1,
  [SSERV_CMD_USER_SEL_CREATE_GROUP_MEMBER_PAGE] = 1,
  [SSERV_CMD_USER_SEL_CREATE_GROUP_MEMBER_ACTION] = 1,
  [SSERV_CMD_USER_SEL_DELETE_GROUP_MEMBER_PAGE] = 1,
  [SSERV_CMD_USER_SEL_DELETE_GROUP_MEMBER_ACTION] = 1,
  [SSERV_CMD_USER_IMPORT_CSV_PAGE] = 1,
  [SSERV_CMD_USER_IMPORT_CSV_ACTION] = 1,
  [SSERV_CMD_GROUP_BROWSE_PAGE] = 1,
  [SSERV_CMD_GROUP_FILTER_CHANGE_ACTION] = 1,
  [SSERV_CMD_GROUP_FILTER_FIRST_PAGE_ACTION] = 1,
  [SSERV_CMD_GROUP_FILTER_PREV_PAGE_ACTION] = 1,
  [SSERV_CMD_GROUP_FILTER_NEXT_PAGE_ACTION] = 1,
  [SSERV_CMD_GROUP_FILTER_LAST_PAGE_ACTION] = 1,
  [SSERV_CMD_GROUP_CREATE_PAGE] = 1,
  [SSERV_CMD_GROUP_CREATE_ACTION] = 1,
  [SSERV_CMD_GROUP_MODIFY_PAGE] = 1,
  [SSERV_CMD_GROUP_MODIFY_PAGE_ACTION] = 1,
  [SSERV_CMD_GROUP_MODIFY_ACTION] = 1,
  [SSERV_CMD_GROUP_DELETE_PAGE] = 1,
  [SSERV_CMD_GROUP_DELETE_PAGE_ACTION] = 1,
  [SSERV_CMD_GROUP_DELETE_ACTION] = 1,
  [SSERV_CMD_GROUP_CANCEL_ACTION] = 1,
  [SSERV_CMD_TESTS_MAIN_PAGE] = 1,
  [SSERV_CMD_TESTS_STATEMENT_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_STATEMENT_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_STATEMENT_EDIT_2_ACTION] = 1,
  [SSERV_CMD_TESTS_STATEMENT_EDIT_3_ACTION] = 1,
  [SSERV_CMD_TESTS_STATEMENT_EDIT_4_ACTION] = 1,
  [SSERV_CMD_TESTS_STATEMENT_EDIT_5_ACTION] = 1,
  [SSERV_CMD_TESTS_STATEMENT_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_STATEMENT_DELETE_SAMPLE_ACTION] = 1,
  [SSERV_CMD_TESTS_TESTS_VIEW_PAGE] = 1,
  [SSERV_CMD_TESTS_CHECK_TESTS_PAGE] = 1,
  [SSERV_CMD_TESTS_GENERATE_ANSWERS_PAGE] = 1,
  [SSERV_CMD_TESTS_SOURCE_HEADER_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_SOURCE_HEADER_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_SOURCE_HEADER_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_SOURCE_FOOTER_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_SOURCE_FOOTER_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_SOURCE_FOOTER_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_SOLUTION_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_SOLUTION_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_SOLUTION_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_STYLE_CHECKER_CREATE_PAGE] = 1,
  [SSERV_CMD_TESTS_STYLE_CHECKER_CREATE_ACTION] = 1,
  [SSERV_CMD_TESTS_STYLE_CHECKER_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_STYLE_CHECKER_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_STYLE_CHECKER_DELETE_PAGE] = 1,
  [SSERV_CMD_TESTS_STYLE_CHECKER_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_CHECKER_CREATE_PAGE] = 1,
  [SSERV_CMD_TESTS_CHECKER_CREATE_ACTION] = 1,
  [SSERV_CMD_TESTS_CHECKER_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_CHECKER_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_CHECKER_DELETE_PAGE] = 1,
  [SSERV_CMD_TESTS_CHECKER_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_VALUER_CREATE_PAGE] = 1,
  [SSERV_CMD_TESTS_VALUER_CREATE_ACTION] = 1,
  [SSERV_CMD_TESTS_VALUER_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_VALUER_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_VALUER_DELETE_PAGE] = 1,
  [SSERV_CMD_TESTS_VALUER_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_INTERACTOR_CREATE_PAGE] = 1,
  [SSERV_CMD_TESTS_INTERACTOR_CREATE_ACTION] = 1,
  [SSERV_CMD_TESTS_INTERACTOR_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_INTERACTOR_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_INTERACTOR_DELETE_PAGE] = 1,
  [SSERV_CMD_TESTS_INTERACTOR_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_CHECKER_CREATE_PAGE] = 1,
  [SSERV_CMD_TESTS_TEST_CHECKER_CREATE_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_CHECKER_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_TEST_CHECKER_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_CHECKER_DELETE_PAGE] = 1,
  [SSERV_CMD_TESTS_TEST_CHECKER_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_INIT_CREATE_PAGE] = 1,
  [SSERV_CMD_TESTS_INIT_CREATE_ACTION] = 1,
  [SSERV_CMD_TESTS_INIT_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_INIT_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_INIT_DELETE_PAGE] = 1,
  [SSERV_CMD_TESTS_INIT_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_MAKEFILE_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_MAKEFILE_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_MAKEFILE_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_MAKEFILE_GENERATE_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_MOVE_UP_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_MOVE_DOWN_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_MOVE_TO_SAVED_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_INSERT_PAGE] = 1,
  [SSERV_CMD_TESTS_TEST_INSERT_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_TEST_EDIT_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_DELETE_PAGE] = 1,
  [SSERV_CMD_TESTS_TEST_DELETE_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_UPLOAD_ARCHIVE_1_PAGE] = 1,
  [SSERV_CMD_TESTS_TEST_CHECK_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_GENERATE_ACTION] = 1,
  [SSERV_CMD_TESTS_SAVED_MOVE_UP_ACTION] = 1,
  [SSERV_CMD_TESTS_SAVED_MOVE_DOWN_ACTION] = 1,
  [SSERV_CMD_TESTS_SAVED_DELETE_PAGE] = 1,
  [SSERV_CMD_TESTS_SAVED_MOVE_TO_TEST_ACTION] = 1,
  [SSERV_CMD_TESTS_README_CREATE_PAGE] = 1,
  [SSERV_CMD_TESTS_README_EDIT_PAGE] = 1,
  [SSERV_CMD_TESTS_README_DELETE_PAGE] = 1,
  [SSERV_CMD_TESTS_CANCEL_ACTION] = 1,
  [SSERV_CMD_TESTS_CANCEL_2_ACTION] = 1,
  [SSERV_CMD_TESTS_TEST_DOWNLOAD] = 1,
  [SSERV_CMD_TESTS_TEST_UPLOAD_PAGE] = 1,
  [SSERV_CMD_TESTS_TEST_CLEAR_INF_ACTION] = 1,
  [SSERV_CMD_TESTS_MAKE] = 1,
  [SSERV_CMD_USER_MAP_MAIN_PAGE] = 1,
  [SSERV_CMD_USER_MAP_DELETE_ACTION] = 1,
  [SSERV_CMD_USER_MAP_ADD_ACTION] = 1,
  [SSERV_CMD_CAPS_MAIN_PAGE] = 1,
  [SSERV_CMD_CAPS_DELETE_ACTION] = 1,
  [SSERV_CMD_CAPS_ADD_ACTION] = 1,
  [SSERV_CMD_CAPS_EDIT_PAGE] = 1,
  [SSERV_CMD_CAPS_EDIT_SAVE_ACTION] = 1,
  [SSERV_CMD_CAPS_EDIT_CANCEL_ACTION] = 1,
  [SSERV_CMD_EJUDGE_XML_UPDATE_ACTION] = 1,
  [SSERV_CMD_EJUDGE_XML_CANCEL_ACTION] = 1,
  [SSERV_CMD_EJUDGE_XML_MUST_RESTART] = 1,
  [SSERV_CMD_IMPORT_FROM_POLYGON_PAGE] = 1,
  [SSERV_CMD_IMPORT_FROM_POLYGON_ACTION] = 1,
  [SSERV_CMD_DOWNLOAD_PROGRESS_PAGE] = 1,
  [SSERV_CMD_DOWNLOAD_CLEANUP_ACTION] = 1,
  [SSERV_CMD_DOWNLOAD_KILL_ACTION] = 1,
  [SSERV_CMD_DOWNLOAD_CLEANUP_AND_CHECK_ACTION] = 1,
  [SSERV_CMD_DOWNLOAD_CLEANUP_AND_IMPORT_ACTION] = 1,
  [SSERV_CMD_UPDATE_FROM_POLYGON_PAGE] = 1,
  [SSERV_CMD_UPDATE_FROM_POLYGON_ACTION] = 1,
  [SSERV_CMD_IMPORT_PROBLEMS_BATCH_ACTION] = 1,
  [SSERV_CMD_CREATE_CONTEST_BATCH_ACTION] = 1,
  [SSERV_CMD_IMPORT_CONTEST_FROM_POLYGON_PAGE] = 1,

  [SSERV_CMD_LOGIN_PAGE] = 1,
  [SSERV_CMD_MAIN_PAGE] = 1,
  [SSERV_CMD_CONTEST_PAGE] = 1,
  [SSERV_CMD_CONTEST_XML_PAGE] = 1,
  [SSERV_CMD_SERVE_CFG_PAGE] = 1,
  [SSERV_CMD_CREATE_CONTEST_PAGE] = 1,
  [SSERV_CMD_CREATE_CONTEST_2_ACTION] = 1,
  [SSERV_CMD_CONTEST_ALREADY_EDITED_PAGE] = 1,
  [SSERV_CMD_CONTEST_LOCKED_PAGE] = 1,
  [SSERV_CMD_CHECK_TESTS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_PERMISSIONS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_REGISTER_ACCESS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_USERS_ACCESS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_MASTER_ACCESS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_JUDGE_ACCESS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_TEAM_ACCESS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_SERVE_CONTROL_ACCESS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_USER_FIELDS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_CONTESTANT_FIELDS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_RESERVE_FIELDS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_COACH_FIELDS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_ADVISOR_FIELDS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_GUEST_FIELDS_PAGE] = 1,
  [SSERV_CMD_CNTS_START_EDIT_ACTION] = 1,
  [SSERV_CMD_CNTS_EDIT_CUR_CONTEST_PAGE] = 1,
  [SSERV_CMD_GLOB_EDIT_CONTEST_START_CMD_PAGE] = 1,
  [SSERV_CMD_GLOB_EDIT_CONTEST_STOP_CMD_PAGE] = 1,
  [SSERV_CMD_GLOB_EDIT_STAND_HEADER_FILE_PAGE] = 1,
  [SSERV_CMD_GLOB_EDIT_STAND_FOOTER_FILE_PAGE] = 1,
  [SSERV_CMD_GLOB_EDIT_STAND2_HEADER_FILE_PAGE] = 1,
  [SSERV_CMD_GLOB_EDIT_STAND2_FOOTER_FILE_PAGE] = 1,
  [SSERV_CMD_GLOB_EDIT_PLOG_HEADER_FILE_PAGE] = 1,
  [SSERV_CMD_GLOB_EDIT_PLOG_FOOTER_FILE_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_USERS_HEADER_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_USERS_FOOTER_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_COPYRIGHT_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_WELCOME_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_REG_WELCOME_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_REGISTER_EMAIL_FILE_PAGE] = 1,
  [SSERV_CMD_CNTS_RELOAD_FILE_ACTION] = 1,
  [SSERV_CMD_CNTS_SAVE_FILE_ACTION] = 1,
  [SSERV_CMD_CNTS_CLEAR_FILE_ACTION] = 1,
  [SSERV_CMD_CNTS_EDIT_CUR_GLOBAL_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_CUR_LANGUAGES_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_CUR_PROBLEMS_PAGE] = 1,
  [SSERV_CMD_CNTS_EDIT_CUR_PROBLEM_PAGE] = 1,
  [SSERV_CMD_CNTS_START_EDIT_PROBLEM_ACTION] = 1,
  [SSERV_CMD_CNTS_START_EDIT_VARIANT_ACTION] = 1,
  [SSERV_CMD_CNTS_EDIT_CUR_VARIANT_PAGE] = 1,
  [SSERV_CMD_CNTS_NEW_SERVE_CFG_PAGE] = 1,
  [SSERV_CMD_CNTS_COMMIT_PAGE] = 1,
  [SSERV_CMD_USER_BROWSE_PAGE] = 1,
  [SSERV_CMD_USER_BROWSE_DATA] = 1,
  [SSERV_CMD_GET_CONTEST_LIST] = 1,
  [SSERV_CMD_CNTS_SAVE_BASIC_FORM] = 1,
  [SSERV_CMD_CNTS_SAVE_FLAGS_FORM] = 1,
  [SSERV_CMD_CNTS_SAVE_REGISTRATION_FORM] = 1,
  [SSERV_CMD_CNTS_SAVE_TIMING_FORM] = 1,
  [SSERV_CMD_CNTS_SAVE_URLS_FORM] = 1,
  [SSERV_CMD_CNTS_SAVE_HEADERS_FORM] = 1,
  [SSERV_CMD_CNTS_SAVE_ATTRS_FORM] = 1,
  [SSERV_CMD_CNTS_SAVE_NOTIFICATIONS_FORM] = 1,
  [SSERV_CMD_CNTS_SAVE_ADVANCED_FORM] = 1,
  [SSERV_CMD_GLOB_SAVE_MAIN_FORM] = 1,
  [SSERV_CMD_GLOB_SAVE_CAPABILITIES_FORM] = 1,
  [SSERV_CMD_GLOB_SAVE_FILES_FORM] = 1,
  [SSERV_CMD_GLOB_SAVE_QUOTAS_FORM] = 1,
  [SSERV_CMD_GLOB_SAVE_URLS_FORM] = 1,
  [SSERV_CMD_GLOB_SAVE_ATTRS_FORM] = 1,
  [SSERV_CMD_GLOB_SAVE_ADVANCED_FORM] = 1,
  [SSERV_CMD_GLOB_SAVE_LIMITS_FORM] = 1,
  [SSERV_CMD_LANG_SAVE_MAIN_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_ID_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_FILES_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_VALIDATION_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_VIEW_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_SUBMISSION_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_COMPILING_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_RUNNING_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_LIMITS_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_CHECKING_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_SCORING_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_FEEDBACK_FORM] = 1,
  [SSERV_CMD_PROB_SAVE_STANDING_FORM] = 1,

  [SSERV_CMD_MAIN_PAGE_BUTTON] = 1,

  [SSERV_CMD_MIGRATION_PAGE] = 1,
};
