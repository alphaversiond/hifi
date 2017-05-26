#
#  Created by William Brown and Thijs Wenker on 2016/10/29
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

if (WIN32)
    
    set(TARGET_NAME hifiPrioVR)
    setup_hifi_plugin(Script Qml Widgets)
    link_hifi_libraries(shared controllers ui plugins input-plugins)
    target_priovr()

endif()
