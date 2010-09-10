/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   VoiceController.cpp
 *  @brief  In-world voice controller user interface
 *          
 */
 
#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "VoiceController.h"
#include <CommunicationsService.h>

#include "DebugOperatorNew.h"

namespace CommUI
{
    VoiceController::VoiceController(Communications::InWorldVoice::SessionInterface* voice_session) :
        transmission_mode_(Mute),
        in_world_voice_session_(voice_session),
        voice_activity_timeout_ms_(1000),
        transmitting_audio_(false),
        push_to_talk_on_(false),
        toggle_mode_on_(false)
    {

    }

    VoiceController::~VoiceController()
    {

    }

    void VoiceController::SetTransmissionMode(TransmissionMode mode)
    {
        TransmissionMode old_mode = transmission_mode_;
        transmission_mode_ = mode;
        push_to_talk_on_ = false;
        toggle_mode_on_ = false;
        SetTransmissionState();
        if (old_mode != transmission_mode_)
            emit TransmissionModeChanged(transmission_mode_);
    }

    void VoiceController::SetPushToTalkOn()
    {
        push_to_talk_on_ = true;
        SetTransmissionState();
    }

    void VoiceController::SetPushToTalkOff()
    {
        push_to_talk_on_ = false;
        SetTransmissionState();
    }

    void VoiceController::SetTransmissionState()
    {
        bool was_transmitting_audio = transmitting_audio_;
        switch (transmission_mode_)
        {
        case Mute:
            transmitting_audio_ = false;
            break;

        case ContinuousTransmission:
            transmitting_audio_ = true;
            break;

        case PushToTalk:
            if (push_to_talk_on_)
                transmitting_audio_ = true;
            else
                transmitting_audio_ = false;
            break;

        case ToggleMode:
            if (toggle_mode_on_)
                transmitting_audio_ = true;
            else
                transmitting_audio_ = false;
            break;

        case VoiceActivity:
            transmitting_audio_ = false;
            // @todo IMPLEMENT
            break;
        }

        if (transmitting_audio_)
            in_world_voice_session_->EnableAudioSending();
        else
            in_world_voice_session_->DisableAudioSending();

        if (transmitting_audio_ != was_transmitting_audio)
            if (transmitting_audio_)
                emit TransmittingAudioStarted();
            else
                emit TransmittingAudioStopped();
    }


    VoiceControllerWidget::VoiceControllerWidget(Communications::InWorldVoice::SessionInterface* voice_session) :
        VoiceController(voice_session)

    {
        setupUi(this);

        QStringList options;
        options << "Mute" << "Continuous transmission" << "Push-to-Talk" << "Toggle mode";
        transmissionModeComboBox->addItems(options);   
        QObject::connect(transmissionModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(ApplyTransmissionModeSelection(int)));

        QObject::connect(ShowListButton, SIGNAL(clicked()), this, SLOT(OpenParticipantListWidget()));

        QObject::connect(muteAllCheckBox, SIGNAL(clicked())
    }

    VoiceControllerWidget::~VoiceControllerWidget()
    {

    }

    void VoiceControllerWidget::ApplyTransmissionModeSelection(int selection)
    {
        SetTransmissionMode(TransmissionMode(selection));
    }

    void VoiceControllerWidget::OpenParticipantListWidget()
    {

    }

} // CommUI

