#!/usr/bin/env python3
"""
CellKernel OS Test Suite
Unit tests for all kernel components
"""

import unittest
import sys
import os

# Add src to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

class TestKernel(unittest.TestCase):
    """Test kernel functionality"""
    
    def test_kernel_initialization(self):
        """Test kernel initializes correctly"""
        from kernel.cellkernel import CellKernel
        kernel = CellKernel()
        self.assertEqual(kernel.version, "1.0.0")
        self.assertEqual(kernel.name, "CellKernel Qwen")
        
    def test_kernel_modules(self):
        """Test kernel modules list"""
        from kernel.cellkernel import CellKernel
        kernel = CellKernel()
        self.assertIsInstance(kernel.modules, list)


class TestUI(unittest.TestCase):
    """Test UI functionality"""
    
    def test_ui_platform_detection(self):
        """Test UI platform detection"""
        from ui.interface import CellKernelUI
        ui = CellKernelUI()
        self.assertIn(ui.platform, ['linux', 'darwin', 'windows', 'unknown'])


class TestFilesystem(unittest.TestCase):
    """Test filesystem functionality"""
    
    def test_fs_initialization(self):
        """Test filesystem initializes"""
        from filesystem.vfs import VirtualFilesystem
        fs = VirtualFilesystem()
        fs.initialize()
        self.assertIn('/', fs.root)


class TestNetwork(unittest.TestCase):
    """Test network functionality"""
    
    def test_network_init(self):
        """Test network stack initialization"""
        from network.stack import NetworkStack
        net = NetworkStack()
        net.initialize()
        self.assertIsInstance(net.interfaces, list)


class TestAudio(unittest.TestCase):
    """Test audio functionality"""
    
    def test_audio_volume(self):
        """Test audio volume control"""
        from audio.subsystem import AudioSubsystem
        audio = AudioSubsystem()
        audio.set_volume(75)
        self.assertEqual(audio.volume, 75)


if __name__ == '__main__':
    unittest.main()
