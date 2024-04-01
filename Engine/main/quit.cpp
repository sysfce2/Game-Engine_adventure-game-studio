//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

//
// Quit game procedure
//
#include <stdio.h>
#include "core/platform.h"
#include <allegro.h> // find files, allegro_exit
#include "ac/cdaudio.h"
#include "ac/common.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/roomstatus.h"
#include "ac/route_finder.h"
#include "ac/translation.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/agseditordebugger.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "main/config.h"
#include "main/engine.h"
#include "main/main.h"
#include "main/quit.h"
#include "ac/spritecache.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "core/assetmanager.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "plugin/plugin_engine.h"
#include "script/cc_common.h"
#include "media/audio/audio_system.h"
#include "media/video/video.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern RoomStruct thisroom;
extern RoomStatus troom;    // used for non-saveable rooms, eg. intro
extern char pexbuf[STD_BUFFER_SIZE];
extern int proper_exit;
extern char check_dynamic_sprites_at_exit;
extern int editor_debugging_initialized;
extern IAGSEditorDebugger *editor_debugger;
extern int need_to_stop_cd;
extern int use_cdplayer;
extern IGraphicsDriver *gfxDriver;

bool handledErrorInEditor;

void quit_tell_editor_debugger(const String &qmsg, QuitReason qreason)
{
    if (editor_debugging_initialized)
    {
        if (qreason & kQuitKind_GameException)
            handledErrorInEditor = send_exception_to_debugger(qmsg.GetCStr());
        send_state_to_debugger("EXIT");
        editor_debugger->Shutdown();
    }
}

void quit_stop_cd()
{
    if (need_to_stop_cd)
        cd_manager(3,0);
}

void quit_check_dynamic_sprites(QuitReason qreason)
{
    if ((qreason & kQuitKind_NormalExit) && (check_dynamic_sprites_at_exit) && 
        (game.options[OPT_DEBUGMODE] != 0))
    {
        // Check that the dynamic sprites have been deleted;
        // ignore those that are owned by the game objects.
        for (size_t i = 1; i < spriteset.GetSpriteSlotCount(); i++)
        {
            if ((game.SpriteInfos[i].Flags & SPF_DYNAMICALLOC) &&
                ((game.SpriteInfos[i].Flags & SPF_OBJECTOWNED) == 0))
            {
                debug_script_warn("Dynamic sprite %d was never deleted", i);
            }
        }
    }
}

void quit_shutdown_audio()
{
    set_our_eip(9917);
    game.options[OPT_CROSSFADEMUSIC] = 0;
    shutdown_sound();
}

// Parses the quit message; returns:
// * QuitReason - which is a code of the reason we're quitting (game error, etc);
// * errmsg - a pure error message (extracted from the parsed string).
// * alertis - a complex message to post into the engine output (stdout, log);
QuitReason quit_check_for_error_state(const char *qmsg, String &errmsg, String &alertis)
{
    if (qmsg[0]=='|')
    {
        return kQuit_GameRequest;
    }
    else if (qmsg[0]=='!')
    {
        QuitReason qreason;
        qmsg++;

        if (qmsg[0] == '|')
        {
            qreason = kQuit_UserAbort;
            alertis = "Abort key pressed.\n\n";
        }
        else if (qmsg[0] == '?')
        {
            qmsg++;
            qreason = kQuit_ScriptAbort;
            alertis = "A fatal error has been generated by the script using the AbortGame function. Please contact the game author for support.\n\n";
        }
        else
        {
            qreason = kQuit_GameError;
            alertis.Format("An error has occurred. Please contact the game author for support, as this "
                "is likely to be an error in game logic or script and not a bug in AGS engine.\n"
                "(Engine version %s)\n\n", EngineVersion.LongString.GetCStr());
        }

        alertis.Append(cc_get_error().CallStack);

        if (qreason != kQuit_UserAbort)
        {
            alertis.AppendFmt("\nError: %s", qmsg);
            errmsg = qmsg;
        }
        return qreason;
    }
    else if (qmsg[0] == '%')
    {
        qmsg++;
        alertis.Format("A warning has been generated. This is not normally fatal, but you have selected "
            "to treat warnings as errors.\n"
            "(Engine version %s)\n\n%s\n%s", EngineVersion.LongString.GetCStr(), cc_get_error().CallStack.GetCStr(),
            qmsg);
        errmsg = qmsg;
        return kQuit_GameWarning;
    }
    else
    {
        alertis.Format("An internal error has occurred. Please note down the following information.\n"
            "If the problem persists, contact the game author for support or post these details on the AGS Technical Forum.\n"
            "(Engine version %s)\n"
            "\nError: %s", EngineVersion.LongString.GetCStr(), qmsg);
        return kQuit_FatalError;
    }
}

// quit - exits the engine, shutting down everything gracefully
// The parameter is the message to print. If this message begins with
// an '!' character, then it is printed as a "contact game author" error.
// If it begins with a '|' then it is treated as a "thanks for playing" type
// message. If it begins with anything else, it is treated as an internal
// error.
// "!|" is a special code used to mean that the player has aborted (Alt+X)
void quit(const char *quitmsg)
{
    Debug::Printf(kDbgMsg_Info, "Quitting the game...");

    // NOTE: we must not use the quitmsg pointer past this step,
    // as it may be from a plugin and we're about to free plugins
    String errmsg, fullmsg;
    QuitReason qreason = quit_check_for_error_state(quitmsg, errmsg, fullmsg);

#if defined (AGS_AUTO_WRITE_USER_CONFIG)
    if (qreason & kQuitKind_NormalExit)
        save_config_file();
#endif // AGS_AUTO_WRITE_USER_CONFIG

    handledErrorInEditor = false;

    quit_tell_editor_debugger(errmsg, qreason);

    set_our_eip(9900);

    set_our_eip(9016);

    quit_stop_cd();
    if (use_cdplayer)
        platform->ShutdownCDPlayer();

    set_our_eip(9019);

    video_shutdown();
    quit_shutdown_audio();

    set_our_eip(9908);

    shutdown_pathfinder();

    // Release game data and unregister assets
    quit_check_dynamic_sprites(qreason);
    unload_game();
    AssetMgr.reset();

    // Be sure to unlock mouse on exit, or users will hate us
    sys_window_lock_mouse(false);
    engine_shutdown_gfxmode();

    platform->PreBackendExit();

    // On abnormal exit: display the message (at this point the window still exists)
    if ((qreason & kQuitKind_NormalExit) == 0 && !handledErrorInEditor)
    {
        platform->DisplayAlert("%s", fullmsg.GetCStr());
    }

    // release backed library
    // WARNING: no Allegro objects should remain in memory after this,
    // if their destruction is called later, program will crash!
    shutdown_font_renderer();
    allegro_exit();
    sys_main_shutdown();

    platform->PostBackendExit();

    set_our_eip(9903);

    proper_exit=1;

    Debug::Printf(kDbgMsg_Alert, "***** ENGINE HAS SHUTDOWN");

    shutdown_debug();
    AGSPlatformDriver::Shutdown();

    set_our_eip(9904);
    exit(EXIT_NORMAL);
}

extern "C" {
    void quit_c(char*msg) {
        quit(msg);
    }
}
