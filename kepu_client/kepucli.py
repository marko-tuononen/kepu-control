"""
kepucli -- A client to interact with the kitchen garden (keittiöpuutarha, kepu) controller.
"""
import argparse
import binascii
import logging
import os
import socket
import struct
import sys

DEFAULT_HOST = "192.168.101.102"
DEFAULT_PORT = 29500
DEFAULT_LOG_LEVEL = logging.WARNING
DEFAULT_RECEIVE_BUFSIZE = 1024
PROTOCOL_IDENTIFIER_REQUEST = 0x00
PROTOCOL_IDENTIFIER_RESPONSE = 0x7F
RESPONSE_CODE_OK = 0x00

def parse_args():
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser(
        prog=f"{os.path.basename(__file__)}",
        description="a client to interact with the kitchen garden controller",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "--host",
        type=str,
        default=DEFAULT_HOST,
        help="controller hostname or ipv4 address"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help="controller port"
    )
    parser.add_argument(
        "--log-level",
        type=int,
        default=DEFAULT_LOG_LEVEL,
        help="log level"
    )
    parser.add_argument(
        "--receive-bufsize",
        type=int,
        default=DEFAULT_RECEIVE_BUFSIZE,
        help="size of receive buffer"
    )
    parser.add_argument(
        "--relay0",
        type=bool,
        required=True,
        action=argparse.BooleanOptionalAction,
        help="turn relay0 on/off"
    )
    parser.add_argument(
        "--relay1",
        type=bool,
        required=True,
        action=argparse.BooleanOptionalAction,
        help="turn relay1 on/off"
    )
    return parser.parse_args()

def bytes_to_hex(bytes_array):
    """
    Convert given bytes_array to a hexadecimal string
    """
    return f"0x{binascii.hexlify(bytearray(bytes_array)).decode('utf-8')}"

def build_request(relay0, relay1):
    """
    Build request message (array of bytes)
    """
    return struct.pack(
        "BB",
        PROTOCOL_IDENTIFIER_REQUEST,
        nth_bit(0, relay0) | nth_bit(1, relay1)
    )

def send_and_recv(host, port, bufsize, request):
    """
    Send request to given host and port, receive response from the host
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((host, port))
        logging.info("Connected to %s:%s", host, port)
        logging.debug("socket: %s", sock)
        logging.debug("request: %s", bytes_to_hex(request))
        sock.sendall(request)
        logging.info("Sent request: %d bytes", len(request))
        response = sock.recv(bufsize)
        logging.info("Received response: %d bytes", len(response))
        logging.debug("response: %s", bytes_to_hex(response))
        return response

def parse_response(response):
    """
    Parse and validate response message (array of bytes)
    """
    parsed_response = struct.unpack("BBBf", response)
    logging.debug("parsed response %s", parsed_response)
    if parsed_response[0] != PROTOCOL_IDENTIFIER_RESPONSE:
        raise ValueError(
            f"invalid protocol identifier (got {parsed_response[0]}, "
            f"expected {PROTOCOL_IDENTIFIER_RESPONSE})"
        )
    if parsed_response[1] != RESPONSE_CODE_OK:
        raise ValueError(f"response not ok (got code {parsed_response[1]}")
    return parsed_response

def nth_bit(n, is_set):
    """
    Return a number with nth bit set, if is_set. Otherwise, return 0.
    """
    return 1<<n if is_set else 0

def process(args):
    """
    Perform actual processing, handle exceptions.
    """
    results = ()
    try:
        results = parse_response(
            send_and_recv(
                args.host,
                args.port,
                args.receive_bufsize,
                build_request(args.relay0, args.relay1)
            )
        )
    except Exception:
        logging.exception("exception")

    return results

def main(args):
    """
    Main function, takes command-line arguments as a parameter
    """
    logging.getLogger().setLevel(args.log_level)
    logging.debug("args: %s", args)
    results = process(args)
    if results:
        logging.warning("Relays switched %s. Current temperature %f", hex(results[-2]), results[-1])

    return 0 if results else 1

if __name__ == "__main__":
    sys.exit(main(parse_args()))
