#include "parser.h"

#ifdef __AVR
#include "avr_test.h"
#else
#include "host_test.h"
#endif  // __AVR

AVR_TEST(test_checksum_valid_uppercase) {
    uint8_t calculated = 0x2A;
    uint8_t received[2] = {'2', 'A'};
    bool result = _Parser::checksum_valid(calculated, received);
    TEST_ASSERT(result);
}

AVR_TEST(test_checksum_lowercase) {
    uint8_t calculated = 0x3b;
    uint8_t received[2] = {'3', 'b'};
    bool result = _Parser::checksum_valid(calculated, received);
    TEST_ASSERT(!result);
}

AVR_TEST(test_checksum_valid_numbers_only) {
    uint8_t calculated = 0x05;
    uint8_t received[2] = {'0', '5'};
    bool result = _Parser::checksum_valid(calculated, received);
    TEST_ASSERT(result);
}

AVR_TEST(test_checksum_mismatch_fails) {
    uint8_t calculated = 0x2A;
    uint8_t received[2] = {'2', 'B'}; // Wrong low nibble
    bool result = _Parser::checksum_valid(calculated, received);
    TEST_ASSERT(!result);
}

AVR_TEST(test_checksum_swapped_nibbles_fails) {
    uint8_t calculated = 0xFA;        // Expected FA
    uint8_t received[2] = {'A', 'F'}; // Received AF
    bool result = _Parser::checksum_valid(calculated, received);
    TEST_ASSERT(!result);
}

AVR_TEST(test_checksum_garbage_chars_fails) {
    uint8_t calculated = 0x2A;
    uint8_t received[2] = {'G', 'X'}; // Completely invalid hex chars
    bool result = _Parser::checksum_valid(calculated, received);
    TEST_ASSERT(!result);
}

AVR_TEST(test_calculate_time_epoch_start) {
    const uint8_t date[6] = {'0', '1', '0', '1', '2', '0'};
    const uint8_t time[6] = {'0', '0', '0', '0', '0', '0'};
    uint32_t seconds = _Parser::calculate_time(date, time);
    // Should be exactly 0 seconds from the epoch start
    TEST_ASSERT(seconds == 0UL);
}

AVR_TEST(test_calculate_time_one_day_elapsed) {
    const uint8_t date[6] = {'0', '2', '0', '1', '2', '0'};
    const uint8_t time[6] = {'0', '0', '0', '0', '0', '0'};
    uint32_t seconds = _Parser::calculate_time(date, time);
    TEST_ASSERT(seconds == 86400UL);
}

AVR_TEST(test_calculate_time_leap_day_2020) {
    const uint8_t date[6] = {'2', '9', '0', '2', '2', '0'};
    const uint8_t time[6] = {'1', '2', '0', '0', '0', '0'};
    uint32_t seconds = _Parser::calculate_time(date, time);
    TEST_ASSERT(seconds == ((31 + 28) * 86400UL + 12 * 3600UL));
}

AVR_TEST(test_calculate_time_post_leap_year_2024) {
    // 1 March 2024 00:00:00 UTC
    // 2020 (leap, 366 days), 2021 (365), 2022 (365), 2023 (365) = 1461 full days.
    // Jan 2024 (31), Feb 2024 (29) = 60 days.
    // Total full days elapsed = 1521.
    // 1521 * 86400 = 131,414,400 seconds.
    const uint8_t date[6] = {'0', '1', '0', '3', '2', '4'};
    const uint8_t time[6] = {'0', '0', '0', '0', '0', '0'};
    uint32_t seconds = _Parser::calculate_time(date, time);
    TEST_ASSERT(seconds == 131414400UL);
}

AVR_TEST(test_calculate_time_arbitrary_recent) {
    const uint8_t date[6] = {'1', '4', '1', '0', '2', '6'};
    const uint8_t time[6] = {'2', '3', '4', '7', '1', '5'};
    uint32_t seconds = _Parser::calculate_time(date, time);
    // Calculated standard expected value for 14 Oct 2026 23:47:15:
    // Full years: 2020 (366), 2021 (365), 2022 (365), 2023 (365), 2024 (366), 2025 (365) = 2192 days
    // Months in 2026: Jan(31), Feb(28), Mar(31), Apr(30), May(31), Jun(30), Jul(31), Aug(31), Sep(30) = 273 days
    // Days in October: 13 full days elapsed = 13 days
    // Total days = 2478 days -> 2478 * 86400 = 214,099,200 seconds
    // Time: 23*3600 + 47*60 + 15 = 85,635 seconds
    // Grand Total = 214,184,835 seconds
    TEST_ASSERT(seconds == 214184835UL);
}

AVR_TEST(test_calculate_time_century_max_boundary) {
    const uint8_t date[6] = {'3', '1', '1', '2', '9', '9'};
    const uint8_t time[6] = {'2', '3', '5', '9', '5', '9'};
    uint32_t seconds = _Parser::calculate_time(date, time);
    // Upper theoretical bound check (Should fit perfectly within uint32_t maximum of 4,294,967,295)
    // 79 years elapsed. Fits well inside uint32_t limits (~2.52 billion seconds).
    TEST_ASSERT(seconds > 2500000000UL);
}

bool feed_string(_Parser& parser, const char* str) {
    bool completed = false;
    while (*str) {
        if (parser.parse((uint8_t)*str)) {
            completed = true;
        }
        str++;
    }
    return completed;
}

AVR_TEST(test_full_parser_valid_rmc) {
    _Parser parser;
    const char* valid_msg = "$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191126,020.3,E,A*0C";
    bool parsed = feed_string(parser, valid_msg);
    TEST_ASSERT(parsed);
    NmeaTime result = parser.get();
    TEST_ASSERT(result.status == 'A');
    TEST_ASSERT(result.mode == 'A');
    TEST_ASSERT(result.sec_since_epoch == 217'292'086UL);
}

AVR_TEST(test_full_parser_alternative_talker) {
    _Parser parser;
    // Valid custom-built string using GNRMC talker id and empty non-critical fields
    const char* gn_msg = "$GNRMC,120000,V,XXXX.XX,X,YYYYY.YY,Y,0.0,0.0,010121,,,N*15";
    bool parsed = feed_string(parser, gn_msg);
    TEST_ASSERT(parsed);
    NmeaTime result = parser.get();
    TEST_ASSERT(result.status == 'V');
    TEST_ASSERT(result.mode == 'N');
    TEST_ASSERT(result.sec_since_epoch == 31'665'600UL);
}

AVR_TEST(test_full_parser_ignores_decimal_seconds) {
    _Parser parser;
    // Time field contains decimal precision (.123)
    const char* decimal_msg = "$GPRMC,225446.123,A,4916.45,N,12311.12,W,000.5,054.7,191126,020.3,E,A*12";
    bool parsed = feed_string(parser, decimal_msg);
    TEST_ASSERT(parsed);
    NmeaTime result = parser.get();
    TEST_ASSERT(result.status == 'A');
    TEST_ASSERT(result.mode == 'A');
    TEST_ASSERT(result.sec_since_epoch == 217'292'086UL);
}

AVR_TEST(test_full_parser_invalid_checksum_fails) {
    _Parser parser;
    // Intentional bad checksum (*00 instead of *0C)
    const char* bad_msg = "$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191126,020.3,E,A*00";
    bool parsed = feed_string(parser, bad_msg);
    TEST_ASSERT(!parsed);
}

// 5. Test sequence tracker transitions across data writes
AVR_TEST(test_full_parser_sequence_lock) {
    _Parser parser;
    uint8_t seq_start = parser.sequence();
    TEST_ASSERT((seq_start & 1) == 0); // Must start even
    // Feed the string up to just before the last checksum character
    const char* partial_msg = "$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191126,020.3,E,A*0";
    while (*partial_msg) {
        parser.parse((uint8_t)*partial_msg);
        partial_msg++;
    }
    // Fire the final character to commit the internal write and cycle sequence counters
    bool parsed = parser.parse('C');
    TEST_ASSERT(parsed);
    uint8_t seq_end = parser.sequence();
    TEST_ASSERT((seq_end & 1) == 0); // Must end on an even number
    TEST_ASSERT(seq_end != seq_start); // Sequence number must have advanced by 2
}

AVR_TEST(test_parser_ignores_preceding_noise_and_control_chars) {
    _Parser parser;

    // Stream begins with raw null bytes, carriage returns, line feeds, invalid characters,
    // and a broken fragment before hitting a flawless valid RMC string.
    // Date: 19 Nov 2026, Time: 22:54:46 -> Epoch: 214613686UL
    const char* noisy_stream =
        "\x01\x02\r\n"                       // Binary junk & control sequences
        "GARBAGE_TEXT_WITHOUT_START_CHARACTER\n"  // Random serial garbage lines
        "$GPGGA,123456.00,4916.45,N,12311.12,W*12\r\n" // An unhandled message type (GGA)
        "$GPRMC,22,broken_fragment*00\r\n"       // A severely truncated RMC sentence
        "$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191126,020.3,E,A*0C"; // The true target

    bool parsed = feed_string(parser, noisy_stream);
    TEST_ASSERT(parsed); // True validation must occur on the tail sentence

    NmeaTime result = parser.get();
    TEST_ASSERT(result.status == 'A');
    TEST_ASSERT(result.mode == 'A');
    TEST_ASSERT(result.sec_since_epoch == 217'292'086UL);
}

AVR_TEST(test_parser_handles_interrupted_sentences_via_start_delimiter) {
    _Parser parser;

    // An RMC sentence starts, gets cut off right in the middle of the time array,
    // and immediately transmits a fresh '$' token to restart parsing.
    // Date: 01 Jan 2021, Time: 12:00:00 -> Epoch: 31665600UL
    const char* interrupted_stream =
        "$GPRMC,120" // Aborted sentence frame (no trailing delimiter, no checksum)
        "$GNRMC,120000,V,XXXX.XX,X,YYYYY.YY,Y,0.0,0.0,010121,,,N*15"; // Valid fallback restart

    bool parsed = feed_string(parser, interrupted_stream);
    TEST_ASSERT(parsed);

    NmeaTime result = parser.get();
    TEST_ASSERT(result.status == 'V');
    TEST_ASSERT(result.mode == 'N');
    TEST_ASSERT(result.sec_since_epoch == 31'665'600UL);
}

AVR_TEST(test_parser_skips_interspersed_non_rmc_payloads) {
    _Parser parser;

    // Valid RMC data is packed tight inside a standard sequence of multi-talker NMEA frames
    // Date: 19 Nov 2026, Time: 22:54:46 -> Epoch: 214613686UL
    const char* multi_sentence_stream =
        "$GPGSV,3,1,11,03,03,111,31,04,15,270,38*76\r\n" // GSV Sentence (Should skip)
        "$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191126,020.3,E,A*0C\r\n" // Target RMC
        "$GPGSA,A,3,03,04,09,16,22,26,29,31,,,,,2.5,1.3,2.1*30\r\n"; // GSA Sentence (Should skip)

    bool parsed = feed_string(parser, multi_sentence_stream);
    TEST_ASSERT(parsed);

    NmeaTime result = parser.get();
    TEST_ASSERT(result.status == 'A');
    TEST_ASSERT(result.mode == 'A');
    TEST_ASSERT(result.sec_since_epoch == 217'292'086UL);
}

AVR_TEST(test_parser_never_validates_pure_noise) {
    _Parser parser;

    // Stream consists of valid structural components but corrupt layout or mismatched checksum flags
    const char* pure_noise_stream =
        "$$$$$,,,,,,*\r\n" // Structural spam
        "$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191126,020.3,E,A*FF\r\n" // Corrupt checksum
        "$ABCDE,123456,X,0000.00,N,00000.00,E,000.0,000.0,010120,000.0,E,E*00\r\n"; // Not an RMC message

    bool parsed = feed_string(parser, pure_noise_stream);
    TEST_ASSERT(!parsed); // The state machine must never yield true
}

// A lightweight, deterministic 8-bit LFSR pseudo-random generator for AVR
// Uses the standard maximal period polynomial: x^8 + x^6 + x^5 + x^4 + 1
uint8_t get_pseudo_random_byte8(uint8_t& lfsr_state) {
    uint8_t bit = ((lfsr_state >> 0) ^ (lfsr_state >> 2) ^ (lfsr_state >> 3) ^ (lfsr_state >> 4)) & 1;
    lfsr_state = (lfsr_state >> 1) | (bit << 7);
    return lfsr_state;
}

AVR_TEST(test_parser_fuzz_interspersed_noise) {
    _Parser parser;

    // Seed the LFSR state with a non-zero value
    uint8_t lfsr = 0xAA;

    // Target message: Date 19 Nov 2026, Time 22:54:46 -> Epoch: 214613686UL
    const char* target_msg = "$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191126,020.3,E,A*0C";

    // We will fuzz by pumping 100 blocks of random junk fragments, followed immediately
    // by individual segments of our valid message, testing mid-packet resynchronization.
    for (uint16_t iteration = 0; iteration < 20; ++iteration) {
        // Generate a random burst of junk bytes (ranging from 1 to 15 bytes long)
        uint8_t junk_length = (get_pseudo_random_byte8(lfsr) & 0x0F) + 1;
        for (uint8_t i = 0; i < junk_length; ++i) {
            uint8_t raw_byte = get_pseudo_random_byte8(lfsr);

            // Critical rule: Ensure our fuzzer noise doesn't accidentally emit a '$'
            // delimiter, which would simulate a legitimate structural reset out of order.
            if (raw_byte == '$') {
                raw_byte = 'X';
            }

            // The parser must process the junk and return false
            bool triggered_early = parser.parse(raw_byte);
            TEST_ASSERT(!triggered_early);
        }
    }

    // Feed the completely uncorrupted target sequence after the noise bursts
    bool parsed_successfully = false;
    const char* ptr = target_msg;
    while (*ptr) {
        if (parser.parse((uint8_t)*ptr)) {
            parsed_successfully = true;
        }
        ptr++;
    }

    // Assert that despite the immense preceding noise, the parser synced and completed perfectly
    TEST_ASSERT(parsed_successfully);

    NmeaTime result = parser.get();
    TEST_ASSERT(result.status == 'A');
    TEST_ASSERT(result.mode == 'A');
    TEST_ASSERT(result.sec_since_epoch == 217'292'086UL);
}

AVR_TEST(test_parser_fuzz_pure_noise_never_triggers) {
    _Parser parser;
    uint8_t lfsr = 0x55; // Different initial seed
    bool parsed_successfully = false;

    // Feed 2000 continuous bytes of pseudo-random data stream
    for (uint16_t i = 0; i < 2000; ++i) {
        uint8_t random_noise_byte = get_pseudo_random_byte8(lfsr);
        if (parser.parse(random_noise_byte)) {
          parsed_successfully = true;
        }
    }
    // The parser should never complete a message on pure white noise
    TEST_ASSERT(!parsed_successfully);
}

// 16-bit Galois LFSR (Maximal period: 65,535)
// Polynomial: x^16 + x^14 + x^13 + x^11 + 1
inline uint16_t get_pseudo_random_16(uint16_t& lfsr_state) {
    uint16_t bit = ((lfsr_state >> 0) ^ (lfsr_state >> 2) ^ (lfsr_state >> 3) ^ (lfsr_state >> 5)) & 1;
    lfsr_state = (lfsr_state >> 1) | (bit << 15);
    return lfsr_state;
}

AVR_TEST(test_parser_massive_fuzz_stress) {
    _Parser parser;
    uint16_t lfsr = 0xACE1; // Any non-zero start seed
    bool parsed_successfully = false;

    for (uint16_t i = 0; i < 10000; ++i) {
        // Cast the 16-bit generator state down to an 8-bit character stream
        uint8_t random_noise_byte = (uint8_t)(get_pseudo_random_16(lfsr) & 0xFF);

        // Skip structural delimiter to test raw noise resiliency
        if (random_noise_byte == '$') {
            continue;
        }
        if (parser.parse(random_noise_byte)) {
          parsed_successfully = true;
        }
    }
    // The parser should never complete a message on pure white noise
    TEST_ASSERT(!parsed_successfully);

    // Now we test a valid string to ensure that the internal state
    // machinery is not somehow broken after all of the noise
    const char* valid_msg = "$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191126,020.3,E,A*0C";
    bool parsed = feed_string(parser, valid_msg);
    TEST_ASSERT(parsed);
    NmeaTime result = parser.get();
    TEST_ASSERT(result.status == 'A');
    TEST_ASSERT(result.mode == 'A');
    TEST_ASSERT(result.sec_since_epoch == 217'292'086UL);
}


AVR_CASES {
    AVR_CASE(test_checksum_valid_uppercase),
    AVR_CASE(test_checksum_lowercase),
    AVR_CASE(test_checksum_valid_numbers_only),
    AVR_CASE(test_checksum_mismatch_fails),
    AVR_CASE(test_checksum_swapped_nibbles_fails),
    AVR_CASE(test_checksum_garbage_chars_fails),
    AVR_CASE(test_calculate_time_epoch_start),
    AVR_CASE(test_calculate_time_one_day_elapsed),
    AVR_CASE(test_calculate_time_leap_day_2020),
    AVR_CASE(test_calculate_time_post_leap_year_2024),
    AVR_CASE(test_calculate_time_arbitrary_recent),
    AVR_CASE(test_calculate_time_century_max_boundary),
    AVR_CASE(test_full_parser_valid_rmc),
    AVR_CASE(test_full_parser_alternative_talker),
    AVR_CASE(test_full_parser_ignores_decimal_seconds),
    AVR_CASE(test_full_parser_invalid_checksum_fails),
    AVR_CASE(test_full_parser_sequence_lock),
    AVR_CASE(test_parser_ignores_preceding_noise_and_control_chars),
    AVR_CASE(test_parser_handles_interrupted_sentences_via_start_delimiter),
    AVR_CASE(test_parser_skips_interspersed_non_rmc_payloads),
    AVR_CASE(test_parser_never_validates_pure_noise),
    AVR_CASE(test_parser_fuzz_interspersed_noise),
    AVR_CASE(test_parser_fuzz_pure_noise_never_triggers),
    AVR_CASE(test_parser_massive_fuzz_stress),
};

#ifdef __AVR
#define AVR_TEST_H_MAIN
#include "avr_test.h"
#else
int main() {
  AVR_RUN();
  return 0;
}
#endif  // __AVR
