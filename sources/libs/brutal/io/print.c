#include <brutal/base/attributes.h>
#include <brutal/debug/locked.h>
#include <brutal/io/fmt.h>
#include <brutal/io/funcs.h>
#include <brutal/io/print.h>
#include <brutal/parse/scan.h>

PrintValue print_val_signed(FmtInt val)
{
    return (PrintValue){nullstr, PRINT_SIGNED, {._signed = val}};
}

PrintValue print_val_unsigned(FmtUInt val)
{
    return (PrintValue){nullstr, PRINT_UNSIGNED, {._unsigned = val}};
}

#ifndef __freestanding__
PrintValue print_val_float(MAYBE_UNUSED double val)
{
    return (PrintValue){nullstr, PRINT_FLOAT, {._float = val}};
}
#endif

PrintValue print_val_cstring(char const *val)
{
    return (PrintValue){nullstr, PRINT_STRING, {._str = str$(val)}};
}

PrintValue print_val_char(char val)
{
    return (PrintValue){nullstr, PRINT_CHAR, {._char = val}};
}

PrintValue print_val_str(Str val)
{
    return (PrintValue){nullstr, PRINT_STRING, {._str = val}};
}

PrintValue print_val_pointer(void *ptr)
{
    return (PrintValue){nullstr, PRINT_POINTER, {._pointer = ptr}};
}

int print_color_id(FmtColor color)
{
    int result = 0;

    switch (color.type)
    {
    case FMT_BLACK:
        result = 0;
        break;
    case FMT_RED:
        result = 1;
        break;
    case FMT_GREEN:
        result = 2;
        break;
    case FMT_YELLOW:
        result = 3;
        break;
    case FMT_BLUE:
        result = 4;
        break;
    case FMT_MAGENTA:
        result = 5;
        break;
    case FMT_CYAN:
        result = 6;
        break;
    case FMT_WHITE:
        return 7 + 60;
    case FMT_GRAY:
        result = 7;
        break;
    case FMT_BLACK_GRAY:
        return 0 + 60;
    case FMT_COL_NONE:
        return -1;
    }

    if (color.bright)
    {
        return result + 60;
    }
    return result;
}

static IoResult print_start_style(IoWriter writer, FmtStyle style)
{
    size_t written = 0;
    if (style.bold)
    {
        written += TRY(IoResult, io_write_str(writer, str$("\033[1m")));
    }
    if (style.underline)
    {
        written += TRY(IoResult, io_write_str(writer, str$("\033[4m")));
    }

    int fg_id = print_color_id(style.fg_color);
    if (fg_id != -1)
    {
        written += TRY(IoResult, io_write_str(writer, str$("\033[")));
        written += TRY(IoResult, fmt_signed((Fmt){}, writer, fg_id + 30));
        written += TRY(IoResult, io_write_str(writer, str$("m")));
    }

    int bg_id = print_color_id(style.bg_color);
    if (bg_id != -1)
    {
        written += TRY(IoResult, io_write_str(writer, str$("\033[")));
        written += TRY(IoResult, fmt_signed((Fmt){}, writer, bg_id + 40));
        written += TRY(IoResult, io_write_str(writer, str$("m")));
    }

    return OK(IoResult, written);
}

static IoResult print_end_style(IoWriter writer)
{
    return io_write_str(writer, str$("\033[0m"));
}

IoResult print_dispatch(IoWriter writer, Fmt fmt, PrintValue value)
{
    switch (value.type)
    {

    case PRINT_SIGNED:
        if (fmt.type == FMT_CHAR)
        {
            return fmt_char(fmt, writer, value._unsigned);
        }
        else
        {
            return fmt_signed(fmt, writer, value._signed);
        }

    case PRINT_UNSIGNED:
        if (fmt.type == FMT_CHAR)
        {
            return fmt_char(fmt, writer, value._unsigned);
        }
        else
        {
            return fmt_unsigned(fmt, writer, value._unsigned);
        }
#ifndef __freestanding__
    case PRINT_FLOAT:
        return fmt_float(fmt, writer, value._float);
#endif
    case PRINT_STRING:
        return fmt_str(fmt, writer, value._str);

    case PRINT_POINTER:
        return fmt_unsigned(fmt, writer, (uintptr_t)value._pointer);

    case PRINT_CHAR:
        return fmt_char(fmt, writer, value._char);
    default:
        panic$("No formater for value of type {}", value.type)
    }
}

IoResult print_impl(IoWriter writer, Str format, PrintArgs args)
{
    size_t current = 0;
    size_t written = 0;
    bool skip_fmt = false;
    Scan scan;
    scan_init(&scan, format);

    while (!scan_ended(&scan))
    {
        if (scan_skip_word(&scan, str$("{{")))
        {
            skip_fmt = false;
            written += TRY(IoResult, io_write_byte(writer, '{'));
        }
        else if (scan_skip_word(&scan, str$("}}")))
        {
            skip_fmt = false;
            written += TRY(IoResult, io_write_byte(writer, '}'));
        }
        else if (scan_curr(&scan) == '{' && !skip_fmt)
        {
            Fmt fmt = fmt_parse(&scan);

            if (fmt.style.has_style)
            {
                written += TRY(IoResult, print_start_style(writer, fmt.style));
            }

            if (current < args.count)
            {
                written += TRY(IoResult, print_dispatch(writer, fmt, args.values[current]));
            }
            else
            {
                written += TRY(IoResult, io_write_str(writer, str$("{}")));
            }

            if (fmt.style.has_style)
            {
                written += TRY(IoResult, print_end_style(writer));
            }
            current++;
        }
        else if (scan_curr(&scan) == '\\' && skip_fmt == false)
        {
            skip_fmt = true;
            scan_next(&scan);
        }
        else
        {
            skip_fmt = false;
            written += TRY(IoResult, io_write_byte(writer, scan_next(&scan)));
        }
    }

    return OK(IoResult, written);
}

Str str_fmt_impl(Alloc *alloc, Str fmt, PrintArgs args)
{
    Buf buf;
    buf_init(&buf, fmt.len, alloc);
    print_impl(buf_writer(&buf), fmt, args);
    return buf_str(&buf);
}
