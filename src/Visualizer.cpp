/*
 * Visualizer.cpp
 *
 * Created on: Jul 30, 2015
 *     Author: dpayne
 */

#include "Visualizer.h"
#include "Source/MpdAudioSource.h"
#include "Domain/VisConstants.h"
#include "Domain/VisException.h"
#include "Transformer/SpectrumTransformer.h"
#include "Transformer/SgsTransformer.h"
#include "Transformer/MonsterCatTransformer.h"
#include "Writer/NcursesWriter.h"
#include "Utils/NcursesUtils.h"

#include <iostream>
#include <ncurses.h>

vis::Visualizer::Visualizer(const vis::Settings *const settings)
    : m_current_audio_source_index{0}, m_current_transformer_index{0},
      m_shutdown{false}, m_settings{settings}, m_pcm_buffer{nullptr}
{
    m_pcm_buffer = static_cast<pcm_stereo_sample *>(
        calloc(m_settings->get_sample_size(), sizeof(pcm_stereo_sample)));
}

void vis::Visualizer::add_audio_source(const std::string &audio_source)
{
    if (audio_source == VisConstants::k_mpd_audio_source_name)
    {
        m_audio_sources.emplace_back(std::unique_ptr<vis::MpdAudioSource>{
            new vis::MpdAudioSource{m_settings}});
    }
}

void vis::Visualizer::run()
{
    setup_audio_sources();

    setup_transformers();

    auto audioSource = get_current_audio_source();
    auto transformer = get_current_transformer();
    m_writer = std::unique_ptr<NcursesWriter>{new NcursesWriter{m_settings}};
    while (!should_shutdown() &&
           audioSource->read(m_pcm_buffer, m_settings->get_sample_size()))
    {
        process_user_input();
        if (m_settings->is_stereo_enabled())
        {
            transformer->execute_stereo(m_pcm_buffer, m_writer.get());
        }
        else
        {
            transformer->execute_mono(m_pcm_buffer, m_writer.get());
        }

        // update sources and transformers
        audioSource = get_current_audio_source();
        transformer = get_current_transformer();
    }
}

void vis::Visualizer::process_user_input()
{
    const int32_t user_input = NcursesUtils::get_user_input();
    switch (user_input)
    {
    case ' ':
        // switch visualizer
        m_current_transformer_index =
            (m_current_transformer_index + 1) % m_transformers.size();
        break;
    default:
        // do nothing
        break;
    }
}

void vis::Visualizer::setup_audio_sources()
{
    for (const auto &audioSource : m_settings->get_audio_sources())
    {
        add_audio_source(audioSource);
    }

    // Throw an error if there are no audio sources
    if (m_audio_sources.size() < 1)
    {
        throw vis::VisException{"No audio sources defined"};
    }
}

void vis::Visualizer::setup_transformers()
{
    m_transformers.emplace_back(std::unique_ptr<vis::MonsterCatTransformer>{
        new vis::MonsterCatTransformer{m_settings}});
    m_transformers.emplace_back(std::unique_ptr<vis::SpectrumTransformer>{
        new vis::SpectrumTransformer{m_settings}});
    m_transformers.emplace_back(std::unique_ptr<vis::SgsTransformer>{
        new vis::SgsTransformer{m_settings}});
}

vis::Visualizer::~Visualizer()
{
    if (m_pcm_buffer != nullptr)
    {
        free(m_pcm_buffer);
    }
}
