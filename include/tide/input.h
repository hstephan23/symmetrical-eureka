#ifndef TIDE_INPUT_H
#define TIDE_INPUT_H

typedef enum TideInputResult {
    TIDE_INPUT_NONE = 0,
    TIDE_INPUT_PENDING,
    TIDE_INPUT_EVENT,
    TIDE_INPUT_INVALID
} TideInputResult;

typedef enum TideInputEventType {
    TIDE_INPUT_KEY = 1,
    TIDE_INPUT_TEXT
} TideInputEventType;

typedef enum TideKey {
    TIDE_KEY_UNKNOWN = 0,
    TIDE_KEY_ESCAPE,
    TIDE_KEY_ENTER,
    TIDE_KEY_BACKSPACE,
    TIDE_KEY_CTRL_C,
    TIDE_KEY_CTRL_Q,
    TIDE_KEY_CTRL_S,
    TIDE_KEY_CTRL_P,
    TIDE_KEY_ARROW_UP,
    TIDE_KEY_ARROW_DOWN,
    TIDE_KEY_ARROW_RIGHT,
    TIDE_KEY_ARROW_LEFT
} TideKey;

typedef struct TideInputEvent {
    TideInputEventType type;
    TideKey key;
    unsigned char text;
} TideInputEvent;

typedef enum TideInputState {
    TIDE_INPUT_STATE_NORMAL = 0,
    TIDE_INPUT_STATE_ESC,
    TIDE_INPUT_STATE_CSI
} TideInputState;

typedef struct TideInputParser {
    TideInputState state;
} TideInputParser;

void tide_input_parser_init(TideInputParser *parser);
TideInputResult tide_input_feed(TideInputParser *parser, unsigned char byte, TideInputEvent *event);
TideInputResult tide_input_flush(TideInputParser *parser, TideInputEvent *event);

#endif
