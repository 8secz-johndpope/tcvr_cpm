//
//  Skybox.cpp
//  libraries/graphics/src/graphics
//
//  Created by Sam Gateau on 5/4/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Skybox.h"


#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <ViewFrustum.h>
#include <shaders/Shaders.h>
#include "ShaderConstants.h"

using namespace graphics;

Skybox::Skybox() {
    Schema schema;
    _schemaBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Schema), (const gpu::Byte*) &schema));
}

void Skybox::setColor(const Color& color) {
    _schemaBuffer.edit<Schema>().color = color;
    _empty = false;
}

void Skybox::setCubemap(const gpu::TexturePointer& cubemap) {
    _cubemap = cubemap;
    if (cubemap) {
        _empty = false;
    }
}

void Skybox::setOrientation(const glm::quat& orientation) {
    // The zone rotations need to be negated
    _orientation = orientation;
    _orientation.w = -_orientation.w;
}

void Skybox::updateSchemaBuffer() const {
    auto blend = 0.0f;
    if (getCubemap() && getCubemap()->isDefined()) {
        blend = 0.5f;

        // If pitch black neutralize the color
        if (glm::all(glm::equal(getColor(), glm::vec3(0.0f)))) {
            blend = 1.0f;
        }
    }

    if (blend != _schemaBuffer.get<Schema>().blend) {
        _schemaBuffer.edit<Schema>().blend = blend;
    }
}

void Skybox::clear() {
    _schemaBuffer.edit<Schema>().color = vec3(0);
    _cubemap = nullptr;
    _empty = true;
}

void Skybox::prepare(gpu::Batch& batch) const {
    batch.setUniformBuffer(graphics::slot::buffer::SkyboxParams, _schemaBuffer);
    gpu::TexturePointer skymap = getCubemap();
    // FIXME: skymap->isDefined may not be threadsafe
    if (skymap && skymap->isDefined()) {
        batch.setResourceTexture(graphics::slot::texture::Skybox, skymap);
    }
}

void Skybox::render(gpu::Batch& batch, const ViewFrustum& frustum) const {
    updateSchemaBuffer();
    Skybox::render(batch, frustum, (*this));
}

void Skybox::render(gpu::Batch& batch, const ViewFrustum& viewFrustum, const Skybox& skybox) {
    // Create the static shared elements used to render the skybox
    static gpu::BufferPointer theConstants;
    static gpu::PipelinePointer thePipeline;
    static std::once_flag once;
    std::call_once(once, [&] {
        {
            auto skyShader = gpu::Shader::createProgram(shader::graphics::program::skybox);
            auto skyState = std::make_shared<gpu::State>();
            // Must match PrepareStencil::STENCIL_BACKGROUND
            const int8_t STENCIL_BACKGROUND = 0;
            skyState->setStencilTest(true, 0xFF, gpu::State::StencilTest(STENCIL_BACKGROUND, 0xFF, gpu::EQUAL,
                gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

            thePipeline = gpu::Pipeline::create(skyShader, skyState);
        }
    });


    // Render
    glm::mat4 projMat;
    viewFrustum.evalProjectionMatrix(projMat);

    Transform viewTransform;
    viewFrustum.evalViewTransform(viewTransform);

    // Orientate view transform to be relative to zone
    viewTransform.setRotation(skybox.getOrientation() * viewTransform.getRotation());

    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewTransform);
    batch.setModelTransform(Transform()); // only for Mac

    batch.setPipeline(thePipeline);
    skybox.prepare(batch);
    batch.draw(gpu::TRIANGLE_STRIP, 4);

    batch.setResourceTexture(graphics::slot::texture::Skybox, nullptr);
}
