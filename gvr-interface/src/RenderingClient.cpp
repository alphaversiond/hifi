//
//  RenderingClient.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/20/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QThread>
#include <QtWidgets/QInputDialog>

#include <AddressManager.h>
#include <AudioClient.h>
#include <AvatarHashMap.h>
#include <NodeList.h>

#include "RenderingClient.h"

RenderingClient* RenderingClient::_instance = NULL;

RenderingClient::RenderingClient(QObject *parent, const QString& launchURLString) :
    Client(parent)
{
    _instance = this;
    
    // connect to AddressManager and pass it the launch URL, if we have one
    auto addressManager = DependencyManager::get<AddressManager>();
    connect(addressManager.data(), &AddressManager::locationChangeRequired, this, &RenderingClient::goToLocation);
    addressManager->loadSettings(launchURLString);
    
    // tell the NodeList which node types all rendering clients will want to know about
    DependencyManager::get<NodeList>()->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer);

    DependencyManager::set<AvatarHashMap>();
    
    // get our audio client setup on its own thread
    QThread* audioThread = new QThread();
    auto audioClient = DependencyManager::set<AudioClient>();
    
    audioClient->setPositionGetter(getPositionForAudio);
    audioClient->setOrientationGetter(getOrientationForAudio);
    
    audioClient->moveToThread(audioThread);
    connect(audioThread, &QThread::started, audioClient.data(), &AudioClient::start);
    connect(audioClient.data(), &AudioClient::destroyed, audioThread, &QThread::quit);
    connect(audioThread, &QThread::finished, audioThread, &QThread::deleteLater);

    audioThread->start();
    
    _avatarTimer.setInterval(16); // 60 FPS
    _avatarTimer.start();
    _fakeAvatar.setDisplayName("GearVR");
    //TODO: CHECK AND REMOVE: _fakeAvatar.setFaceModelURL(QUrl(DEFAULT_HEAD_MODEL_URL));
    //_fakeAvatar.setSkeletonModelURL(QUrl(DEFAULT_BODY_MODEL_URL));
    _fakeAvatar.toByteArray(true, true); // Creates HeadData // TODO: check these true, true
    _fakeAvatar.sendAvatarDataPacket();
}

void RenderingClient::cleanupBeforeQuit() {
    
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(),
                              "stop", Qt::BlockingQueuedConnection);
    
    // destroy the AudioClient so it and its thread will safely go down
    DependencyManager::destroy<AudioClient>();
}

void RenderingClient::goToLocation(const glm::vec3& newPosition,
                                   bool hasOrientationChange, const glm::quat& newOrientation,
                                   bool shouldFaceLocation) {
    qDebug().nospace() << "RenderingClient goToLocation - moving to " << newPosition.x << ", "
       << newPosition.y << ", " << newPosition.z;

    glm::vec3 shiftedPosition = newPosition;

    if (hasOrientationChange) {
       qDebug().nospace() << "RenderingClient goToLocation - new orientation is "
           << newOrientation.x << ", " << newOrientation.y << ", " << newOrientation.z << ", " << newOrientation.w;

       // orient the user to face the target
       glm::quat quatOrientation = newOrientation;

       if (shouldFaceLocation) {

           quatOrientation = newOrientation * glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));

           // move the user a couple units away
           const float DISTANCE_TO_USER = 2.0f;
           shiftedPosition = newPosition - quatOrientation * glm::vec3( 0.0f, 0.0f,-1.0f) * DISTANCE_TO_USER;
       }

       _orientation = quatOrientation;
    }

    _position = shiftedPosition;
    
}
