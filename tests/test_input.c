#include "tide/input.h"
#include "test_support.h"

static void feed_one(TideInputParser *parser, unsigned char byte, TideInputEvent *event)
{
    TideInputResult result = tide_input_feed(parser, byte, event);
    TIDE_ASSERT(result == TIDE_INPUT_EVENT);
}

static void test_printable_text_event(void)
{
    TideInputParser parser;
    TideInputEvent event;

    tide_input_parser_init(&parser);
    feed_one(&parser, 'x', &event);

    TIDE_ASSERT(event.type == TIDE_INPUT_TEXT);
    TIDE_ASSERT(event.text == 'x');
}

static void test_ctrl_s_event(void)
{
    TideInputParser parser;
    TideInputEvent event;

    tide_input_parser_init(&parser);
    feed_one(&parser, 0x13, &event);

    TIDE_ASSERT(event.type == TIDE_INPUT_KEY);
    TIDE_ASSERT(event.key == TIDE_KEY_CTRL_S);
}

static void test_arrow_left_event_can_arrive_across_reads(void)
{
    TideInputParser parser;
    TideInputEvent event;

    tide_input_parser_init(&parser);
    TIDE_ASSERT(tide_input_feed(&parser, 0x1b, &event) == TIDE_INPUT_PENDING);
    TIDE_ASSERT(tide_input_feed(&parser, '[', &event) == TIDE_INPUT_PENDING);
    TIDE_ASSERT(tide_input_feed(&parser, 'D', &event) == TIDE_INPUT_EVENT);
    TIDE_ASSERT(event.type == TIDE_INPUT_KEY);
    TIDE_ASSERT(event.key == TIDE_KEY_ARROW_LEFT);
}

static void test_bare_escape_flushes_as_escape_key(void)
{
    TideInputParser parser;
    TideInputEvent event;

    tide_input_parser_init(&parser);
    TIDE_ASSERT(tide_input_feed(&parser, 0x1b, &event) == TIDE_INPUT_PENDING);
    TIDE_ASSERT(tide_input_flush(&parser, &event) == TIDE_INPUT_EVENT);
    TIDE_ASSERT(event.type == TIDE_INPUT_KEY);
    TIDE_ASSERT(event.key == TIDE_KEY_ESCAPE);
}

int main(void)
{
    test_printable_text_event();
    test_ctrl_s_event();
    test_arrow_left_event_can_arrive_across_reads();
    test_bare_escape_flushes_as_escape_key();
    return 0;
}
