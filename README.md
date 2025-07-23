RP2040-NINO: A Non-Interactive, Non-Random Oracle

Firmware for the Raspberry Pi Pico (RP2040) that implements a deterministic, non-interactive oracle. It generates consistent, repeatable responses to challenges without relying on network connectivity, ideal for offline authentication and cryptographic protocols.

This repository contains the firmware for implementing a Non-Interactive, Non-Random Oracle (NINO) on the Raspberry Pi Pico and other RP2040-based microcontrollers.

This project provides a simple yet powerful implementation of a deterministic oracle. It accepts a challenge via a standard topology and returns a consistent, verifiable response. The core logic is designed to be offline, requiring no network interaction, and produces the same output for the same input every time.

Tags: rp2040, raspberry-pi-pico, oracle, cryptography, deterministic, non-interactive, embedded, firmware
