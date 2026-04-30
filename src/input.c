#include "tide/input.h"

static TideInputResult emit_key(TideInputEvent *event, TideKey key)
{
    event->type = TIDE_INPUT_KEY;
    event->key = key;
    event->text = 0;
    return TIDE_INPUT_EVENT;
}

static TideInputResult emit_text(TideInputEvent *event, unsigned char text)
{
    event->type = TIDE_INPUT_TEXT;
    event->key = TIDE_KEY_UNKNOWN;
    event->text = text;
    return TIDE_INPUT_EVENT;
}

void tide_input_parser_init(TideInputParser *parser)
{
    parser->state = TIDE_INPUT_STATE_NORMAL;
}

TideInputResult tide_input_feed(TideInputParser *parser, unsigned char byte, TideInputEvent *event)
{
    if (parser->state == TIDE_INPUT_STATE_ESC) {
        if (byte == '[') {
            parser->state = TIDE_INPUT_STATE_CSI;
            return TIDE_INPUT_PENDING;
        }
        parser->state = TIDE_INPUT_STATE_NORMAL;
        return TIDE_INPUT_INVALID;
    }

    if (parser->state == TIDE_INPUT_STATE_CSI) {
        parser->state = TIDE_INPUT_STATE_NORMAL;
        switch (byte) {
        case 'A':
            return emit_key(event, TIDE_KEY_ARROW_UP);
        case 'B':
            return emit_key(event, TIDE_KEY_ARROW_DOWN);
        case 'C':
            return emit_key(event, TIDE_KEY_ARROW_RIGHT);
        case 'D':
            return emit_key(event, TIDE_KEY_ARROW_LEFT);
        default:
            return TIDE_INPUT_INVALID;
        }
    }

    switch (byte) {
    case 0x1b:
        parser->state = TIDE_INPUT_STATE_ESC;
        return TIDE_INPUT_PENDING;
    case '\r':
    case '\n':
        return emit_key(event, TIDE_KEY_ENTER);
    case 0x7f:
    case 0x08:
        return emit_key(event, TIDE_KEY_BACKSPACE);
    case 0x03:
        return emit_key(event, TIDE_KEY_CTRL_C);
    case 0x11:
        return emit_key(event, TIDE_KEY_CTRL_Q);
    case 0x13:
        return emit_key(event, TIDE_KEY_CTRL_S);
    case 0x10:
        return emit_key(event, TIDE_KEY_CTRL_P);
    default:
        if (byte >= 0x20 && byte != 0x7f) {
            return emit_text(event, byte);
        }
        return TIDE_INPUT_NONE;
    }
}

TideInputResult tide_input_flush(TideInputParser *parser, TideInputEvent *event)
{
    if (parser->state == TIDE_INPUT_STATE_ESC) {
        parser->state = TIDE_INPUT_STATE_NORMAL;
        return emit_key(event, TIDE_KEY_ESCAPE);
    }

    parser->state = TIDE_INPUT_STATE_NORMAL;
    return TIDE_INPUT_NONE;
}
