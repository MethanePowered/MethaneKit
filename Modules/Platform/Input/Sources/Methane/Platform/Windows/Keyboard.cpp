/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Platform/Windows/Keyboard.h
Windows platform specific types and implementation of Keyboard abstractions.

******************************************************************************/

#include <Methane/Platform/Keyboard.h>
#include <Methane/Instrumentation.h>

#include <map>

namespace Methane::Platform::Keyboard
{

Key KeyConverter::GetKeyByNativeCode(const NativeKey& native_key)
{
    ITT_FUNCTION_TASK();
    static const std::map<uint32_t, Key> s_key_by_native_code =
    {
        { 0x00B, Key::Num0          },
        { 0x002, Key::Num1          },
        { 0x003, Key::Num2          },
        { 0x004, Key::Num3          },
        { 0x005, Key::Num4          },
        { 0x006, Key::Num5          },
        { 0x007, Key::Num6          },
        { 0x008, Key::Num7          },
        { 0x009, Key::Num8          },
        { 0x00A, Key::Num9          },
        { 0x01E, Key::A             },
        { 0x030, Key::B             },
        { 0x02E, Key::C             },
        { 0x020, Key::D             },
        { 0x012, Key::E             },
        { 0x021, Key::F             },
        { 0x022, Key::G             },
        { 0x023, Key::H             },
        { 0x017, Key::I             },
        { 0x024, Key::J             },
        { 0x025, Key::K             },
        { 0x026, Key::L             },
        { 0x032, Key::M             },
        { 0x031, Key::N             },
        { 0x018, Key::O             },
        { 0x019, Key::P             },
        { 0x010, Key::Q             },
        { 0x013, Key::R             },
        { 0x01F, Key::S             },
        { 0x014, Key::T             },
        { 0x016, Key::U             },
        { 0x02F, Key::V             },
        { 0x011, Key::W             },
        { 0x02D, Key::X             },
        { 0x015, Key::Y             },
        { 0x02C, Key::Z             },

        { 0x028, Key::Apostrophe    },
        { 0x02B, Key::BackSlash     },
        { 0x033, Key::Comma         },
        { 0x00D, Key::Equal         },
        { 0x029, Key::GraveAccent   },
        { 0x01A, Key::LeftBracket   },
        { 0x00C, Key::Minus         },
        { 0x034, Key::Period        },
        { 0x01B, Key::RightBracket  },
        { 0x027, Key::Semicolon     },
        { 0x035, Key::Slash         },
        { 0x056, Key::World2        },

        { 0x00E, Key::Backspace     },
        { 0x153, Key::Delete        },
        { 0x14F, Key::End           },
        { 0x01C, Key::Enter         },
        { 0x001, Key::Escape        },
        { 0x147, Key::Home          },
        { 0x152, Key::Insert        },
        { 0x15D, Key::Menu          },
        { 0x151, Key::PageDown      },
        { 0x149, Key::PageUp        },
        { 0x045, Key::Pause         },
        { 0x146, Key::Pause         },
        { 0x039, Key::Space         },
        { 0x00F, Key::Tab           },
        { 0x03A, Key::CapsLock      },
        { 0x145, Key::NumLock       },
        { 0x046, Key::ScrollLock    },
        { 0x03B, Key::F1            },
        { 0x03C, Key::F2            },
        { 0x03D, Key::F3            },
        { 0x03E, Key::F4            },
        { 0x03F, Key::F5            },
        { 0x040, Key::F6            },
        { 0x041, Key::F7            },
        { 0x042, Key::F8            },
        { 0x043, Key::F9            },
        { 0x044, Key::F10           },
        { 0x057, Key::F11           },
        { 0x058, Key::F12           },
        { 0x064, Key::F13           },
        { 0x065, Key::F14           },
        { 0x066, Key::F15           },
        { 0x067, Key::F16           },
        { 0x068, Key::F17           },
        { 0x069, Key::F18           },
        { 0x06A, Key::F19           },
        { 0x06B, Key::F20           },
        { 0x06C, Key::F21           },
        { 0x06D, Key::F22           },
        { 0x06E, Key::F23           },
        { 0x076, Key::F24           },
        { 0x038, Key::LeftAlt       },
        { 0x01D, Key::LeftControl   },
        { 0x02A, Key::LeftShift     },
        { 0x15B, Key::LeftSuper     },
        { 0x137, Key::PrintScreen   },
        { 0x138, Key::RightAlt      },
        { 0x11D, Key::RightControl  },
        { 0x036, Key::RightShift    },
        { 0x15C, Key::RightSuper    },
        { 0x150, Key::Down          },
        { 0x14B, Key::Left          },
        { 0x14D, Key::Right         },
        { 0x148, Key::Up            },

        { 0x052, Key::KeyPad0       },
        { 0x04F, Key::KeyPad1       },
        { 0x050, Key::KeyPad2       },
        { 0x051, Key::KeyPad3       },
        { 0x04B, Key::KeyPad4       },
        { 0x04C, Key::KeyPad5       },
        { 0x04D, Key::KeyPad6       },
        { 0x047, Key::KeyPad7       },
        { 0x048, Key::KeyPad8       },
        { 0x049, Key::KeyPad9       },
        { 0x04E, Key::KeyPadAdd     },
        { 0x053, Key::KeyPadDecimal },
        { 0x135, Key::KeyPadDivide  },
        { 0x11C, Key::KeyPadEnter   },
        { 0x059, Key::KeyPadEqual   },
        { 0x037, Key::KeyPadMultiply},
        { 0x04A, Key::KeyPadSubtract},
    };


    // The Ctrl keys require special handling
    if (native_key.w_param == VK_CONTROL)
    {
        MSG  next = {};
        LONG time = 0;

        // Right side keys have the extended key bit set
        if (native_key.l_param & 0x01000000)
            return Key::Unknown;

        // HACK: Alt Gr sends Left Ctrl and then Right Alt in close sequence
        //       We only want the Right Alt message, so if the next message is
        //       Right Alt we ignore this (synthetic) Left Ctrl message
        time = GetMessageTime();

        if (PeekMessageW(&next, NULL, 0, 0, PM_NOREMOVE))
        {
            if (next.message == WM_KEYDOWN ||
                next.message == WM_SYSKEYDOWN ||
                next.message == WM_KEYUP ||
                next.message == WM_SYSKEYUP)
            {
                if (next.wParam == VK_MENU && (next.lParam & 0x01000000) && next.time == time)
                {
                    // Next message is Right Alt down so discard this
                    return Key::Unknown;
                }
            }
        }

        return Key::LeftControl;
    }

    if (native_key.w_param == VK_PROCESSKEY)
    {
        // IME notifies that keys have been filtered by setting the virtual key-code to VK_PROCESSKEY
        return Key::Unknown;
    }

    const uint32_t native_key_code = static_cast<uint32_t>(HIWORD(native_key.l_param) & 0x1FF);
    auto native_code_and_key_it = s_key_by_native_code.find(native_key_code);
    return native_code_and_key_it == s_key_by_native_code.end() ? Key::Unknown : native_code_and_key_it->second;
}

Modifier::Mask KeyConverter::GetModifiersByNativeCode(const NativeKey& native_key)
{
    ITT_FUNCTION_TASK();
    switch (native_key.w_param)
    {
    case VK_CONTROL: return Modifier::Value::Control;
    case VK_SHIFT:   return Modifier::Value::Shift;
    case VK_CAPITAL: return Modifier::Value::CapsLock;
    default:         return Modifier::Value::None;
    }
}

} // namespace Methane::Platform::Keyboard
