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

FILE: Methane/Platform/MacOS/Keyboard.mm
MacOS platform specific types and implementation of Keyboard abstractions.

******************************************************************************/

#include <Methane/Platform/Keyboard.h>
#include <Methane/Instrumentation.h>

#import <AppKit/AppKit.h>

#include <map>

namespace Methane::Platform::Keyboard
{

Key KeyConverter::GetKeyByNativeCode(const NativeKey& native_key)
{
    ITT_FUNCTION_TASK();
    static const std::map<uint8_t, Key> s_key_by_native_code = {
        { 0x1D, Key::Num0           },
        { 0x12, Key::Num1           },
        { 0x13, Key::Num2           },
        { 0x14, Key::Num3           },
        { 0x15, Key::Num4           },
        { 0x17, Key::Num5           },
        { 0x16, Key::Num6           },
        { 0x1A, Key::Num7           },
        { 0x1C, Key::Num8           },
        { 0x19, Key::Num9           },
        
        { 0x00, Key::A              },
        { 0x0B, Key::B              },
        { 0x08, Key::C              },
        { 0x02, Key::D              },
        { 0x0E, Key::E              },
        { 0x03, Key::F              },
        { 0x05, Key::G              },
        { 0x04, Key::H              },
        { 0x22, Key::I              },
        { 0x26, Key::J              },
        { 0x28, Key::K              },
        { 0x25, Key::L              },
        { 0x2E, Key::M              },
        { 0x2D, Key::N              },
        { 0x1F, Key::O              },
        { 0x23, Key::P              },
        { 0x0C, Key::Q              },
        { 0x0F, Key::R              },
        { 0x01, Key::S              },
        { 0x11, Key::T              },
        { 0x20, Key::U              },
        { 0x09, Key::V              },
        { 0x0D, Key::W              },
        { 0x07, Key::X              },
        { 0x10, Key::Y              },
        { 0x06, Key::Z              },
    
        { 0x27, Key::Apostrophe     },
        { 0x2A, Key::BackSlash      },
        { 0x2B, Key::Comma          },
        { 0x18, Key::Equal          },
        { 0x32, Key::GraveAccent    },
        { 0x21, Key::LeftBracket    },
        { 0x1B, Key::Minus          },
        { 0x2F, Key::Period         },
        { 0x1E, Key::RightBracket   },
        { 0x29, Key::Semicolon      },
        { 0x2C, Key::Slash          },
        { 0x0A, Key::World1         },
        
        { 0x7B, Key::Left           },
        { 0x7C, Key::Right          },
        { 0x7E, Key::Up             },
        { 0x7D, Key::Down           },
        
        { 0x73, Key::Home           },
        { 0x77, Key::End            },
        { 0x74, Key::PageUp         },
        { 0x79, Key::PageDown       },
        { 0x72, Key::Insert         },
        { 0x75, Key::Delete         },
        { 0x24, Key::Enter          },
        { 0x35, Key::Escape         },
        { 0x31, Key::Space          },
        { 0x30, Key::Tab            },
        
        { 0x7A, Key::F1             },
        { 0x78, Key::F2             },
        { 0x63, Key::F3             },
        { 0x76, Key::F4             },
        { 0x60, Key::F5             },
        { 0x61, Key::F6             },
        { 0x62, Key::F7             },
        { 0x64, Key::F8             },
        { 0x65, Key::F9             },
        { 0x6D, Key::F10            },
        { 0x67, Key::F11            },
        { 0x6F, Key::F12            },
        { 0x69, Key::F13            },
        { 0x6B, Key::F14            },
        { 0x71, Key::F15            },
        { 0x6A, Key::F16            },
        { 0x40, Key::F17            },
        { 0x4F, Key::F18            },
        { 0x50, Key::F19            },
        { 0x5A, Key::F20            },

        { 0x3A, Key::LeftAlt        },
        { 0x3B, Key::LeftControl    },
        { 0x38, Key::LeftShift      },
        { 0x37, Key::LeftSuper      },
        { 0x3D, Key::RightAlt       },
        { 0x3E, Key::RightControl   },
        { 0x3C, Key::RightShift     },
        { 0x36, Key::RightSuper     },
        { 0x6E, Key::Menu           },
        { 0x47, Key::NumLock        },
        
        { 0x52, Key::KeyPad0        },
        { 0x53, Key::KeyPad1        },
        { 0x54, Key::KeyPad2        },
        { 0x55, Key::KeyPad3        },
        { 0x56, Key::KeyPad4        },
        { 0x57, Key::KeyPad5        },
        { 0x58, Key::KeyPad6        },
        { 0x59, Key::KeyPad7        },
        { 0x5B, Key::KeyPad8        },
        { 0x5C, Key::KeyPad9        },
        { 0x45, Key::KeyPadAdd      },
        { 0x41, Key::KeyPadDecimal  },
        { 0x4B, Key::KeyPadDivide   },
        { 0x4C, Key::KeyPadEnter    },
        { 0x51, Key::KeyPadEqual    },
        { 0x43, Key::KeyPadMultiply },
        { 0x4E, Key::KeyPadSubtract },
    };
    
    auto native_code_and_key_it = s_key_by_native_code.find(native_key.code);
    return native_code_and_key_it == s_key_by_native_code.end() ? Key::Unknown : native_code_and_key_it->second;
}

Modifier::Mask KeyConverter::GetModifiersByNativeCode(const NativeKey& native_key)
{
    ITT_FUNCTION_TASK();
    Modifier::Mask modifiers_mask = Modifier::Value::None;
    
    if (native_key.flags & NSEventModifierFlagShift)
        modifiers_mask |= Modifier::Value::Shift;
    
    if (native_key.flags & NSEventModifierFlagControl)
        modifiers_mask |= Modifier::Value::Control;
    
    if (native_key.flags & NSEventModifierFlagOption)
        modifiers_mask |= Modifier::Value::Alt;
    
    if (native_key.flags & NSEventModifierFlagCommand)
        modifiers_mask |= Modifier::Value::Super;
    
    if (native_key.flags & NSEventModifierFlagCapsLock)
        modifiers_mask |= Modifier::Value::CapsLock;
    
    if (native_key.flags & NSEventModifierFlagNumericPad)
        modifiers_mask |= Modifier::Value::NumLock;
    
    return modifiers_mask;
}

} // namespace Methane::Platform::Keyboard