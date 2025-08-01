//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <stdio.h>
#include <string.h>
#include <allegro.h>
#include "ac/game_version.h"
#include "debug/debug_log.h"
#include "script/cc_common.h"
#include "script/runtimescriptvalue.h"
#include "script/script_api.h"

using namespace AGS::Common;

enum FormatParseResult
{
    kFormatParseNone,
    kFormatParseInvalid,
    kFormatParseArgInteger,
    kFormatParseArgFloat,
    kFormatParseArgCharacter,
    kFormatParseArgString,
    kFormatParseArgPointer,

    kFormatParseArgFirst = kFormatParseArgInteger,
    kFormatParseArgLast = kFormatParseArgPointer
};

// Helper functions for getting parameter value either from script val array or va_list
inline int GetArgInt(const RuntimeScriptValue *sc_args, va_list *varg_ptr, int arg_idx)
{
    if (varg_ptr)
        return va_arg(*varg_ptr, int);
    else
        return sc_args[arg_idx].IValue;
}

inline float GetArgFloat(const RuntimeScriptValue *sc_args, va_list *varg_ptr, int arg_idx)
{
    // note that script variables store only floats, but va_list has floats promoted to double
    if (varg_ptr)
        return (float)va_arg(*varg_ptr, double);
    else
        return sc_args[arg_idx].FValue;
}

inline const char *GetArgPtr(const RuntimeScriptValue *sc_args, va_list *varg_ptr, int arg_idx)
{
    if (varg_ptr)
        return va_arg(*varg_ptr, const char*);
    else
        return reinterpret_cast<const char*>(sc_args[arg_idx].GetPtrWithOffset());
}

static bool AssertFormat(FormatParseResult fmt_type, ScriptValueType value_type)
{
    switch (fmt_type)
    {
    case kFormatParseArgInteger:
        switch (value_type)
        {
        case kScValInteger:
        case kScValPluginArg:
            return true; // acceptable
        default:
            return false;
        }
    case kFormatParseArgFloat:
        switch (value_type)
        {
        case kScValFloat:
        case kScValPluginArg:
            return true; // acceptable
        default:
            return false;
        }
    case kFormatParseArgCharacter:
        switch (value_type)
        {
        case kScValInteger:
        case kScValPluginArg:
            return true; // acceptable
        default:
            return false;
        }
    case kFormatParseArgString:
        switch (value_type)
        {
        case kScValPluginArg:    // for const char* returned from plugin
        case kScValPluginArgPtr: // for const char* returned from plugin
        case kScValData:         // could be a old-style string
        case kScValStringLiteral:// for const char* from script
        case kScValScriptObject: // for String type; TODO: can we narrow this down without damaging performance?
        case kScValPluginObject: // for String type; TODO: can we narrow this down without damaging performance?
            return true; // acceptable
        default:
            return false;
        }
    case kFormatParseArgPointer:
        switch (value_type)
        {
        case kScValPluginArg:    // may contain a pointer
        case kScValPluginArgPtr:
        case kScValData:
        case kScValStringLiteral:
        case kScValScriptObject:
        case kScValPluginObject:
            return true; // acceptable
        default:
            return false;
        }
    default:
        return false;
    }
}

// TODO: this implementation can be further optimised by either not calling
// snprintf but formatting values ourselves, or by using some library method
// that supports customizing, such as getting arguments in a custom way.
size_t ScriptSprintf(char *buffer, size_t buf_length, const char *format,
                          const RuntimeScriptValue *sc_args, int32_t sc_argc, va_list *varg_ptr)
{
    assert((!buffer && buf_length == 0u) || (buffer && buf_length > 0u));
    assert(format);
    assert(sc_args || varg_ptr || sc_argc == 0);
    if (!((!buffer && buf_length == 0u) || (buffer && buf_length > 0u)) || !(format) || !(sc_args || varg_ptr || sc_argc == 0))
    {
        return 0u;
    }

    // Only print warnings if we are doing a printing pass (avoid duplicate warnings when counting buf length);
    // only do type checks if we have script args (possible to assert).
    const bool print_warnings = buffer != nullptr;
    const bool warn_bad_type = print_warnings && (sc_args != nullptr);

    // Prepare pointers to the read/write positions and buffer ends
    char       *out_ptr    = buffer;
    const char *out_endptr = buffer + buf_length;
    const char *fmt_ptr    = format;
    int32_t    arg_idx     = 0;

    // Expected format character count:
    // percent sign:    1
    // flag:            1
    // field width      10 (an uint32 number)
    // precision sign   1
    // precision        10 (an uint32 number)
    // length modifier  2
    // type             1
    // NOTE: although width and precision will
    // not likely be defined by a 10-digit
    // number, such case is theoretically valid.
    const size_t placebuf_size = 27;
    char       placebuf[placebuf_size];
    char       *placebuf_ptr;
    char       *placebuf_endptr = placebuf + placebuf_size - 1; // reserve 1 for terminator

    size_t     output_len = 0u; // the total length of the output (not necessarily printed)
    ptrdiff_t  avail_outbuf;
    const char *litsec_at, *litsec_end; // range of literal input section
    FormatParseResult fmt_done;

    // Parse the format string, looking for argument placeholders
    while (*fmt_ptr && (!out_ptr || out_ptr != out_endptr))
    {
        avail_outbuf = out_endptr - out_ptr;
        // Scan until the first placeholder
        litsec_at = fmt_ptr;
        for (; *fmt_ptr && *fmt_ptr != '%'; ++fmt_ptr);
        litsec_end = fmt_ptr;
        // If the next char is '%', this means that there was a escaped "%%";
        // following is a trick that skips each second '%' in a sequence of '%'
        for (; fmt_ptr[0] == '%' && fmt_ptr[1] == '%'; ++litsec_end, fmt_ptr += 2);
        // Copy the literal section to the output
        ptrdiff_t copy_len = litsec_end - litsec_at;
        if (copy_len > 0u)
        {
            output_len += copy_len;
            if (out_ptr)
            {
                copy_len = std::min(copy_len, avail_outbuf - 1); // save 1 for terminator
                memcpy(out_ptr, litsec_at, copy_len);
                out_ptr += copy_len;
            }
            continue; // we want to guarantee that we start with placeholder
        }

        // We have found a placeholder symbol, try to parse format and print an argument
        placebuf_ptr = placebuf;
        *(placebuf_ptr++) = '%';
        fmt_done = kFormatParseNone;

        // Parse placeholder
        while (*(++fmt_ptr) && fmt_done == kFormatParseNone && placebuf_ptr != placebuf_endptr)
        {
            *(placebuf_ptr++) = *fmt_ptr;
            switch (*fmt_ptr)
            {
            case 'd':
            case 'i':
            case 'o':
            case 'u':
            case 'x':
            case 'X':
                fmt_done = kFormatParseArgInteger;
                break;
            case 'c':
                fmt_done = kFormatParseArgCharacter;
                break;
            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
                fmt_done = kFormatParseArgFloat;
                break;
            case 'p':
                fmt_done = kFormatParseArgPointer;
                break;
            case 's':
                fmt_done = kFormatParseArgString;
                break;
            // Following are valid flag characters that may be used in formatting
            case '#':
            case ' ':
            case '+':
            case '*':
            case '-':
            case '.':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                break;
            default: // met invalid placeholder character
                fmt_done = kFormatParseInvalid;
                break;
            }
        }
        *placebuf_ptr = 0; // terminate the placeholder buf

        avail_outbuf = out_endptr - out_ptr;
        // Use placeholder and print the next argument (if available)
        if (fmt_done >= kFormatParseArgFirst && fmt_done <= kFormatParseArgLast &&
            (varg_ptr || arg_idx < sc_argc))
        {
            int snprintf_res = 0;
            // Print the actual value
            switch (fmt_done)
            {
            case kFormatParseArgInteger:
                if (warn_bad_type && !AssertFormat(kFormatParseArgInteger, sc_args[arg_idx].Type))
                    debug_script_warn("WARNING: String format: place %d expects an integer, but a non-integer value is passed.\n\tFormat string:\n\t\"%s\"", arg_idx + 1, format);
                snprintf_res = snprintf(out_ptr, avail_outbuf, placebuf, GetArgInt(sc_args, varg_ptr, arg_idx));
                break;
            case kFormatParseArgFloat:
                if (warn_bad_type && !AssertFormat(kFormatParseArgFloat, sc_args[arg_idx].Type))
                    debug_script_warn("WARNING: String format: place %d expects a float, but a non-float value is passed.\n\tFormat string:\n\t\"%s\"", arg_idx + 1, format);
                snprintf_res = snprintf(out_ptr, avail_outbuf, placebuf, GetArgFloat(sc_args, varg_ptr, arg_idx));
                break;
            case kFormatParseArgCharacter:
            {
                if (warn_bad_type && !AssertFormat(kFormatParseArgCharacter, sc_args[arg_idx].Type))
                    debug_script_warn("WARNING: String format: place %d expects a character or an integer, but a different value type is passed.\n\tFormat string:\n\t\"%s\"", arg_idx + 1, format);
                int chr = GetArgInt(sc_args, varg_ptr, arg_idx);
                char cbuf[5]{};
                usetc(cbuf, chr);
                snprintf_res = snprintf(out_ptr, avail_outbuf, "%s", cbuf);
                break;
            }
            case kFormatParseArgString:
            {
                const char *p = GetArgPtr(sc_args, varg_ptr, arg_idx);
                if (!p)
                {
                    if (print_warnings)
                        debug_script_warn("WARNING: String format: place %d expects a string, but a null pointer is passed.\n\tFormat string:\n\t\"%s\"", arg_idx + 1, format);
                    p = "(null)"; // explicitly put "(null)" into the placeholder
                }
                else if (p == buffer)
                {
                    cc_error("!ScriptSprintf: formatting argument %d is a pointer to output buffer", arg_idx + 1);
                    return 0u;
                }
                else if (sc_args)
                {
                    // Try to validate the argument type (NOTE: not 100% secure)
                    if (!AssertFormat(kFormatParseArgString, sc_args[arg_idx].Type))
                    {
                        if (warn_bad_type)
                            debug_script_warn("WARNING: String format: place %d expects a string, but a different value type is passed.\n\tFormat string:\n\t\"%s\"", arg_idx + 1, format);
                        p = "(undefined)";
                    }
                }
                snprintf_res = snprintf(out_ptr, avail_outbuf, placebuf, p);
                break;
            }
            case kFormatParseArgPointer:
                if (warn_bad_type && !AssertFormat(kFormatParseArgPointer, sc_args[arg_idx].Type))
                    debug_script_warn("WARNING: String format: place %d expects a pointer, but a different value type is passed.\n\tFormat string:\n\t\"%s\"", arg_idx + 1, format);
                snprintf_res = snprintf(out_ptr, avail_outbuf, placebuf, GetArgPtr(sc_args, varg_ptr, arg_idx));
                break;
            default:
                assert(false); // should not happen
                break;
            }

            arg_idx++;
            output_len += snprintf_res;
            if (out_ptr)
            {
                // snprintf returns maximal number of characters, so limit it with buffer size
                out_ptr += std::min<ptrdiff_t>(snprintf_res, avail_outbuf - 1); // save 1 for terminator
            }
        }
        else
        {
            if (sc_args && (arg_idx >= sc_argc))
            {
                debug_script_warn("WARNING: String format: missing argument %d.\n\tFormat string:\n\t\"%s\"", arg_idx + 1, format);
            }
            else
            {
                debug_script_warn("WARNING: String format: invalid specifier at %d.\n\tFormat string:\n\t\"%s\"", arg_idx + 1, format);
            }

            // If not a supported format, or there are no available parameters,
            // then just copy stored placeholder buffer as-is
            ptrdiff_t copy_len = std::min<ptrdiff_t>(placebuf_ptr - placebuf, placebuf_size - 1);
            output_len += copy_len;
            if (out_ptr)
            {
                copy_len = std::min(copy_len, avail_outbuf - 1); // save 1 for terminator
                memcpy(out_ptr, placebuf, copy_len);
                out_ptr += copy_len;
            }
        }
    }

    // Terminate the string
    if (out_ptr)
    {
        *out_ptr = 0;
    }
    
    return output_len;
}
