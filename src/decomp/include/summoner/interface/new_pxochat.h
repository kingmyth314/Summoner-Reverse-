// new_pxochat.h - screen 11, the PXO chat lobby (new_pxochat.cpp, 0x421e70-0x42540a): the chat banner
// download, the version / login popups, the channel, user and chat lists.
#ifndef NEW_PXOCHAT_H
#define NEW_PXOCHAT_H

// One chat line (0x44 bytes, pxochat_chat_lines 0x5c46c0, 64 of them). Type name ours.
// type: 0 "name text" line (name blue), 1 its continuation, 2 MOTD (gold), 3 magenta, 4 green,
// 5 "name text" with the name in the highlight colour, 6 its continuation.
struct pxochat_line {
    int  type;                        // +0x00
    char text[0x40];                  // +0x04
};

// ---- chat banner ----------------------------------------------------------------------------------
void pxochat_open_banner_url();                     // 0x421e70
void pxo_find_chat_banners();                       // 0x421ea0
void pxo_load_chat_banners();                       // 0x421fa0 one step of the download state machine
void pxochat_leave();                               // 0x422370
void pxo_parse_chat_banners_cfg(int downloaded_only);   // 0x4223d0
bool pxochat_banner_has_url();                      // 0x422790

// ---- the screen -----------------------------------------------------------------------------------
void pxochat_build_popup_text(const char *text1, const char *text2, char *out);   // 0x422980
int  pxochat_add_typed_channel();                   // 0x422b40 0, -1 when empty or the list is full
void pxochat_join_typed_channel();                  // 0x422cf0
void pxochat_input_cancel();                        // 0x422d10
void pxochat_join_selected_channel();               // 0x422d30
void pxochat_update_banner_cursor();                // 0x422d70
void pxochat_load_bitmaps();                        // 0x422dd0
void pxochat_init();                                // 0x422fc0
void pxochat_update_scrollbar(int list);            // 0x423050 0 channels, 1 users, 2 chat, -1 none
void pxochat_leave_thunk();                         // 0x4231a0
void pxochat_update_banner();                       // 0x4231b0
int  pxochat_frame(int key);                        // 0x423220
int  pxochat_checking_version_popup_frame(int key); // 0x423370
int  pxochat_version_error_popup_frame(int key);    // 0x4233d0
int  pxochat_new_version_popup_frame(int key);      // 0x423420
int  pxochat_validating_popup_frame(int key);       // 0x4234a0
int  pxochat_login_error_popup_frame(int key);      // 0x4234f0
void pxochat_version_check_frame();                 // 0x423530
void pxochat_login_frame();                         // 0x4236c0
void pxochat_update_join_buttons();                 // 0x4238c0
int  pxochat_escape_leaves(int key);                // 0x423920
void pxochat_update_status_text();                  // 0x423950
int  pxochat_handle_click(int key);                 // 0x423a20
void pxochat_page_up(int list);                     // 0x424120
void pxochat_page_down(int list);                   // 0x424170
void pxochat_goto_charselect();                     // 0x4241c0
void pxochat_button_action(int button);             // 0x4241d0
void pxochat_input_begin();                         // 0x424320
void pxochat_scroll_up(int list);                   // 0x424360
void pxochat_scroll_down(int list);                 // 0x424390
void pxochat_goto_gamelist();                       // 0x4243e0
void pxochat_show_motd();                           // 0x424400
void pxochat_update_buttons();                      // 0x424410
void pxochat_drag_scrollbar();                      // 0x4244f0
void pxochat_on_channel_joined();                   // 0x4245b0
void pxochat_draw_channel_input();                  // 0x424600
void pxochat_draw();                                // 0x4246d0
void pxochat_draw_banner_highlight();               // 0x4247d0
void pxochat_draw_channel_list();                   // 0x4248c0
void pxochat_draw_user_list();                      // 0x424a60
void pxochat_draw_chat_lines();                     // 0x424b80
void pxochat_draw_scrollbar(int list);              // 0x424e00
void pxochat_draw_chat_input();                     // 0x424ec0
void pxochat_draw_caption();                        // 0x425010

// ---- called by the lobby code -------------------------------------------------------------------------
void pxochat_sync_channel_list();                   // 0x425080
void pxochat_sync_user_list();                      // 0x425110
void pxochat_append_motd(const char *line);         // 0x4251a0 a "!MOTD$#!" line from the server
void pxochat_show_motd_thunk();                     // 0x425250
void pxochat_clear_motd();                          // 0x425260
void pxochat_add_chat_line(const char *text, int type);   // 0x425270
void pxochat_reset_lists();                         // 0x4253e0

// decomp53: set once the PXO login went through; the lobby frame does nothing before (state_mp_lobby_frame).
extern int pxochat_logged_in;                        // 0x5c5924
extern int pxo_run_updater_on_exit;                 // 0x5c5934 set by "Yes" in the new-version popup

#endif
