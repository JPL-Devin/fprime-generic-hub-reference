"""Integration tests for the Generic Hub message round trips."""

import time

import pytest


COMMAND = "FprimeGenericHubReference.DeploymentA.a_comp.HubMessageTest"
COMMAND_TEST = (
    "FprimeGenericHubReference.DeploymentA.c_comp.HubCommandTest"
)

RETURN_EVENTS = {
    0: ("SerDataVerif",),
    1: ("BufDataVerif",),
    2: ("EvrDataVerif",),
    3: ("TlmDataVerif",),
    6: ("SerDataVerif", "BufDataVerif", "EvrDataVerif", "TlmDataVerif"),
}

FAILURE_EVENTS = (
    "SerDataVerifFail",
    "BufDataVerifFail",
    "EvrDataVerifFail",
    "TlmDataVerifFail",
)


def event_name(name):
    return f"FprimeGenericHubReference.DeploymentA.a_comp.{name}"


@pytest.mark.parametrize("message_type", (0, 1, 2, 3, 6))
def test_hub_message_round_trip(fprime_test_api, message_type):
    """Send each supported hub message and verify its return-path event."""
    start = fprime_test_api.get_event_test_history().size()
    fprime_test_api.send_command(COMMAND, [message_type])

    for event in RETURN_EVENTS[message_type]:
        fprime_test_api.assert_event(event_name(event), start=start, timeout=30)

    # Allow delayed failure events to arrive before checking their absence.
    time.sleep(1)
    for event in FAILURE_EVENTS:
        fprime_test_api.assert_event_count(
            0, event_name(event), start=start, timeout=0
        )


def test_hub_command_round_trip(fprime_test_api):
    """Send the hub command through the DeploymentA GDS connection."""
    fprime_test_api.send_command(COMMAND_TEST)
