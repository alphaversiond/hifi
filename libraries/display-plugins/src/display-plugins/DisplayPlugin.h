//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <plugins/DisplayPlugin.h>

#if defined(ANDROID)
#ifndef hifi_display_plugins_DisplayPlugin_h
#define hifi_display_plugins_DisplayPlugin_h

class LibInstance {
public:
  LibInstance(); 
  ~LibInstance() {
    qDebug() << __FILE__ << "has been unloaded";
  }
};

Q_GLOBAL_STATIC(LibInstance, libInstance)

class LibExecutor {
  public: LibExecutor() { libInstance(); }
};
static LibExecutor libExecutor;
#endif
#endif
