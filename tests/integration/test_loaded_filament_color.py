"""Exercise the MK4 color declaration through a running firmware and its GUI.

Uses only the existing simulated PrusaLink password fixture, never a printer.
Run with the MK4 noboot firmware and a compatible Mini404 executable.
"""
import asyncio
import struct

import aiohttp
import pytest

from .actions import encoder, screen, utils
from .test_prusa_link import PRUSALINK_EEPROM, valid_headers, wui_base_url

pytestmark = pytest.mark.asyncio
ENDPOINT = '/api/v1/filaments'
COLOR_EEPROM = {
    **PRUSALINK_EEPROM, 'Language': struct.pack('<H',
                                                ord('n') << 8 | ord('e'))
}


@pytest.fixture
def specific_eeprom_variables():
    return dict(COLOR_EEPROM)


async def wait_for_http(printer):
    # DHCP and PrusaLink startup can finish after the home screen is ready.
    async def probe():
        async with aiohttp.ClientSession(
                base_url=wui_base_url(printer),
                timeout=aiohttp.ClientTimeout(total=3)) as client:
            while True:
                try:
                    async with client.get('/') as response:
                        if response.status == 200:
                            await response.read()
                            return
                except (aiohttp.ClientError, asyncio.TimeoutError):
                    pass
                await asyncio.sleep(1)

    await asyncio.wait_for(probe(), timeout=90)


async def read_slot(client):
    async with client.get(ENDPOINT, headers=valid_headers()) as response:
        assert response.status == 200
        assert response.content_type == 'application/json'
        payload = await response.json()
    assert payload['schema_version'] == 1
    assert len(payload['slots']) == 1
    slot = payload['slots'][0]
    assert slot['slot'] == 0
    assert slot['source'] == 'user_declared'
    return slot


async def rotate(printer, count):
    action = encoder.rotate_right if count >= 0 else encoder.rotate_left
    for _ in range(abs(count)):
        await action(printer)
        await asyncio.sleep(1)


async def enter_menu(printer, moves, menu_check=None):
    await asyncio.wait_for(utils.enter_menu(printer, moves, menu_check),
                           timeout=30)


async def wait_text(printer, text, timeout=20):
    return await asyncio.wait_for(screen.wait_for_text(printer, text), timeout)


async def capture(printer, tmp_path, name):
    image = await screen.take_screenshot(printer)
    image.save(tmp_path / (name + '.png'))


async def test_local_color_api_authentication_and_methods(printer):
    await utils.wait_for_bootstrap(printer)
    await wait_for_http(printer)
    async with aiohttp.ClientSession(
            base_url=wui_base_url(printer),
            timeout=aiohttp.ClientTimeout(total=15)) as client:
        async with client.get(ENDPOINT) as response:
            assert response.status == 401
        async with client.head(ENDPOINT) as response:
            assert response.status == 401
            assert await response.read() == b''
        async with client.get(ENDPOINT,
                              headers={'X-Api-Key':
                                       'wrong-test-key'}) as response:
            assert response.status == 401
        slot = await read_slot(client)
        assert slot['material'] is None
        assert slot['color'] is None
        for method in ('POST', 'PUT', 'DELETE', 'HEAD'):
            async with client.request(method,
                                      ENDPOINT,
                                      headers=valid_headers()) as response:
                assert response.status == 405, method
        async with client.get('/api/v1/filaments/missing',
                              headers=valid_headers()) as response:
            assert response.status == 404
        assert await read_slot(client) == slot


def declaration(color):
    # Production contract: preset PLA tag 1, one atomic RGB/material item.
    value = (1 << 32) | (0 if color is None else 0x01000000 | color)
    return {
        **COLOR_EEPROM, 'Filament Type 0': struct.pack('<B', 1),
        'MK4 Loaded Filament Color v1': struct.pack('<Q', value)
    }


@pytest.mark.parametrize('specific_eeprom_variables, expected_color', [
    pytest.param(declaration(0x000000), '#000000', id='black'),
    pytest.param(declaration(0xffffff), '#FFFFFF', id='white'),
    pytest.param(declaration(None), None, id='unknown'),
])
async def test_confirmed_color_api_and_reboot(printer_factory, expected_color):
    previous = None
    for _ in range(2):
        async with printer_factory() as printer:
            await utils.wait_for_bootstrap(printer)
            await wait_for_http(printer)
            async with aiohttp.ClientSession(
                    base_url=wui_base_url(printer),
                    timeout=aiohttp.ClientTimeout(total=15)) as client:
                slot = await read_slot(client)
            assert slot['material'] == 'PLA'
            assert slot['color'] == expected_color
            if previous is not None:
                assert slot == previous
            previous = slot


@pytest.mark.parametrize('specific_eeprom_variables', [declaration(0x0000ff)])
async def test_palette_choice_back_and_cancel_do_not_confirm_color(
        printer, tmp_path):
    await utils.wait_for_bootstrap(printer)
    await wait_for_http(printer)
    # Home focus starts at Print with the virtual USB stick inserted.
    await enter_menu(printer, 2, 'Load Filament')
    await enter_menu(printer, 1)
    await wait_text(printer, 'already loaded')
    await rotate(printer, -1)  # Existing confirmation defaults to No.
    await encoder.click(printer)
    await wait_text(printer, 'Filament Color')
    await capture(printer, tmp_path, '01-load-material-and-color')
    await rotate(printer, 1)
    await encoder.click(printer)
    await wait_text(printer, 'White')
    await capture(printer, tmp_path, '02-palette')
    # Unknown -> Black -> White -> Gray -> Red.
    await rotate(printer, 4)
    await encoder.click(printer)
    text = await wait_text(printer, '[Red]')
    assert '[red]' in text.lower()
    await capture(printer, tmp_path, '03-red-pending')
    async with aiohttp.ClientSession(
            base_url=wui_base_url(printer),
            timeout=aiohttp.ClientTimeout(total=15)) as client:
        initial = await read_slot(client)
        assert initial['color'] == '#0000FF'
        # Reopen the palette, then select its Return entry above all colors.
        await encoder.click(printer)
        await wait_text(printer, 'White')
        await rotate(printer, -5)
        await encoder.click(printer)
        text = await wait_text(printer, '[Red]')
        assert '[red]' in text.lower()
        # Cancel the material/preheat screen before the physical load begins.
        await rotate(printer, -1)
        await encoder.click(printer)
        await wait_text(printer, 'Load Filament')
        assert await read_slot(client) == initial
    await capture(printer, tmp_path, '04-cancelled')


@pytest.mark.parametrize('specific_eeprom_variables', [declaration(0x0000ff)])
async def test_started_load_invalidates_before_parking_and_after_restart(
        printer_factory, tmp_path):
    async with printer_factory() as printer:
        await utils.wait_for_bootstrap(printer)
        await wait_for_http(printer)
        async with aiohttp.ClientSession(
                base_url=wui_base_url(printer),
                timeout=aiohttp.ClientTimeout(total=15)) as client:
            assert (await read_slot(client))['color'] == '#0000FF'
            await enter_menu(printer, 2, 'Load Filament')
            await enter_menu(printer, 1)
            await wait_text(printer, 'already loaded')
            await rotate(printer, -1)
            await encoder.click(printer)
            await wait_text(printer, 'Filament Color')
            # Start replacing the spool with the same PLA material.
            await rotate(printer, 2)
            await encoder.click(printer)
            await wait_text(printer, 'Parking')
            slot = await read_slot(client)
            assert slot['material'] == 'PLA'
            assert slot['color'] is None
            await capture(printer, tmp_path, '08-started-load-invalidated')
        # Terminating this virtual process models an interruption before success.
    async with printer_factory() as printer:
        await utils.wait_for_bootstrap(printer)
        await wait_for_http(printer)
        async with aiohttp.ClientSession(
                base_url=wui_base_url(printer),
                timeout=aiohttp.ClientTimeout(total=15)) as client:
            assert (await read_slot(client))['color'] is None
        await capture(printer, tmp_path, '09-interrupted-load-reboot')


async def test_successful_load_commits_only_after_color_confirmation(
        printer_factory, tmp_path, pytestconfig):
    if not pytestconfig.getoption('--complete-filament-load'):
        pytest.skip(
            'Complete load is unvalidated: Mini404 v0.9.10 stalls at Parking; use --complete-filament-load with compatible emulation'
        )
    from simulator import Thermistor

    async with printer_factory() as printer:
        await utils.wait_for_bootstrap(printer)
        await wait_for_http(printer)
        await enter_menu(printer, 2, 'Load Filament')
        await enter_menu(printer, 1, 'Filament Color')
        await rotate(printer, 1)
        await encoder.click(printer)
        await wait_text(printer, 'White')
        await rotate(printer, 4)
        await encoder.click(printer)
        await wait_text(printer, '[Red]')
        # These act only on simulated thermistors; normal firmware checks stay on.
        await printer.temperature_set(Thermistor.NOZZLE, 215)
        await printer.temperature_set(Thermistor.BED, 60)
        await rotate(printer, 1)
        await encoder.click(printer)  # Existing PLA material entry.
        await wait_text(printer, 'CONTINUE', timeout=180)
        await capture(printer, tmp_path, '05-insert-filament')
        async with aiohttp.ClientSession(
                base_url=wui_base_url(printer),
                timeout=aiohttp.ClientTimeout(total=15)) as client:
            assert (await read_slot(client))['color'] is None
            await encoder.click(
                printer)  # Continue; the sensor is disabled by the fixture.
            await wait_text(printer, 'Is color correct', timeout=120)
            await capture(printer, tmp_path, '06-confirm-color')
            # Purging/material assignment alone must not confirm the declaration.
            assert (await read_slot(client))['color'] is None
            await encoder.click(printer)  # Existing Yes response.
            await wait_text(printer, 'Load Filament', timeout=60)
            slot = await read_slot(client)
            assert slot['material'] == 'PLA'
            assert slot['color'] == '#FF0000'
        await capture(printer, tmp_path, '07-load-completed')
    async with printer_factory() as printer:
        await utils.wait_for_bootstrap(printer)
        await wait_for_http(printer)
        async with aiohttp.ClientSession(
                base_url=wui_base_url(printer),
                timeout=aiohttp.ClientTimeout(total=15)) as client:
            assert (await read_slot(client))['color'] == '#FF0000'
