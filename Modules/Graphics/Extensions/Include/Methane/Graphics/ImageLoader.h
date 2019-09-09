/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ImageLoader.h
Image Loader creates textures from images loaded via data provider and 
by decoding them from popular image formats.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Data/Provider.h>

#include <string>

namespace Methane::Graphics
{

class ImageLoader final
{
public:
    ImageLoader(Data::Provider& data_provider);

    Texture::Ptr CreateImageTexture(Context& context, const std::string& image_path);

private:
    Data::Provider& m_data_provider;
};

} // namespace Methane::Graphics
