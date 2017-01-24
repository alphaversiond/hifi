"use strict";

//
//  handControllerPointer.js
//  examples/controllers
//
//  Created by Howard Stearns on 2016/04/22
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

// Control the "mouse" using hand controller. (HMD and desktop.)
// First-person only.
// Starts right handed, but switches to whichever is free: Whichever hand was NOT most recently squeezed.
//   (For now, the thumb buttons on both controllers are always on.)
// When partially squeezing over a HUD element, a laser or the reticle is shown where the active hand
// controller beam intersects the HUD.
var logEnabled = false;
function printd(str) {
    if (logEnabled)
        print("anddb-handControllerPointer.js - " + str);
}

printd("start");
printd("including controllers.js");
Script.include("../libraries/controllers.js");

// UTILITIES -------------
//
function ignore() { }

// Utility to make it easier to setup and disconnect cleanly.
function setupHandler(event, handler) {
    event.connect(handler);
    Script.scriptEnding.connect(function () {
        event.disconnect(handler);
    });
}

printd("setupHandler defined");
// If some capability is not available until expiration milliseconds after the last update.
function TimeLock(expiration) {
    var last = 0;
    this.update = function (optionalNow) {
        last = optionalNow || Date.now();
    };
    this.expired = function (optionalNow) {
        return ((optionalNow || Date.now()) - last) > expiration;
    };
}

printd("TimeLock defined");
var handControllerLockOut = new TimeLock(2000);

function Trigger(label) {
    // This part is copied and adapted from handControllerGrab.js. Maybe we should refactor this.
    var that = this;
    that.label = label;
    that.TRIGGER_SMOOTH_RATIO = 0.1; //  Time averaging of trigger - 0.0 disables smoothing
    that.TRIGGER_OFF_VALUE = 0.10;
    that.TRIGGER_ON_VALUE = that.TRIGGER_OFF_VALUE + 0.05;     //  Squeezed just enough to activate search or near grab
    that.rawTriggerValue = 0;
    that.triggerValue = 0;           // rolling average of trigger value
    that.triggerClicked = false;
    that.triggerClick = function (value) {
        printd('[DAYDREAM-CONTROLLER] that.triggerClick ' + value + ' which ' + that.label);
        that.triggerClicked = value;
    };
    that.triggerPress = function (value) {
        printd('[DAYDREAM-CONTROLLER] that.triggerPress ' + value + ' which ' + that.label);
        that.rawTriggerValue = value;
    };
    that.updateSmoothedTrigger = function () { // e.g., call once/update for effect
        var triggerValue = that.rawTriggerValue;
        // smooth out trigger value
        that.triggerValue = (that.triggerValue * that.TRIGGER_SMOOTH_RATIO) +
            (triggerValue * (1.0 - that.TRIGGER_SMOOTH_RATIO));
        OffscreenFlags.navigationFocusDisabled = that.triggerValue != 0.0;
        //printd('[DAYDREAM-CONTROLLER] updateSmoothedTrigger raw[' + that.rawTriggerValue + '] triggerValue[' + that.triggerValue + ']');
    };
    // Current smoothed state, without hysteresis. Answering booleans.
    that.triggerSmoothedClick = function () {
        var clicked = that.triggerClicked;
        //if (clicked || Math.random()>0.988) {
            printd('[DAYDREAM-CONTROLLER] Trigger.triggerSmoothedClick clicked? ' + clicked + ' trigger ' + that.label);
        //}
        return clicked;
    };
    that.triggerSmoothedSqueezed = function () {
        return that.triggerValue > that.TRIGGER_ON_VALUE;
    };
    that.triggerSmoothedReleased = function () {
        return that.triggerValue < that.TRIGGER_OFF_VALUE;
    };

    // This part is not from handControllerGrab.js
    that.state = null; // tri-state: falsey, 'partial', 'full'
    that.update = function () { // update state, called from an update function
        var state = that.state;
        that.updateSmoothedTrigger();

        // The first two are independent of previous state:
        if (that.triggerSmoothedClick()) {
            printd('[DAYDREAM-CONTROLLER] that.triggerSmoothedClick state to full' +  ' which ' + that.label );
            state = 'full';
        } else if (that.triggerSmoothedReleased()) {
            //if (state!==null || Math.random()>0.988) {
                printd('[DAYDREAM-CONTROLLER] that.triggerSmoothedReleased state to null' +  ' which ' + that.label);
            //}
            state = null;
        } else if (that.triggerSmoothedSqueezed()) {
            // Another way to do this would be to have hysteresis in this branch, but that seems to make things harder to use.
            // In particular, the vive has a nice detent as you release off of full, and we want that to be a transition from
            // full to partial.
            printd('[DAYDREAM-CONTROLLER] that.triggerSmoothedSqueezed state to partial' +  ' which ' + that.label);
            state = 'partial';
        }
        that.state = state;
        //if (state || Math.random()>0.98) {
            printd('Trigger.update state ' + that.state + ' trigger ' + that.label);
        //}
    };
    // Answer a controller source function (answering either 0.0 or 1.0).
    that.partial = function () {
        return that.state ? 1.0 : 0.0; // either 'partial' or 'full'
    };
    that.full = function () {
        return (that.state === 'full') ? 1.0 : 0.0;
    };
}

printd("Trigger defined");
// VERTICAL FIELD OF VIEW ---------
//
// Cache the verticalFieldOfView setting and update it every so often.
var verticalFieldOfView, DEFAULT_VERTICAL_FIELD_OF_VIEW = 45; // degrees
function updateFieldOfView() {
    verticalFieldOfView = Settings.getValue('fieldOfView') || DEFAULT_VERTICAL_FIELD_OF_VIEW;
}

printd("updateFieldOfView defined");
// SHIMS ----------
//
var weMovedReticle = false;
function ignoreMouseActivity() {
    // For Android we'll just ignore any Mouse Activity because the one generated by Qt is misleading (generates max x max y mouse movements)
    return true;
    /*var pos = Reticle.position;
    printd("ignoreMouseActivity START with: allowMouseCapture: " + Reticle.allowMouseCapture + " weMovedReticle: " + weMovedReticle + " Reticle.position: " + pos.x + " , " + pos.y);
    // If we're paused, or if change in cursor position is from this script, not the hardware mouse.
    if (!Reticle.allowMouseCapture) {
        printd("ignoreMouseActivity RETURNED true by !Reticle.allowMouseCapture");
        return true;
    }
    //var pos = Reticle.position;
    if (!pos || (pos.x == -1 && pos.y == -1)) {
        printd("ignoreMouseActivity RETURNED true by pos stuff");
        return true;
    }
    // Only we know if we moved it, which is why this script has to replace depthReticle.js
    if (!weMovedReticle) {
        printd("ignoreMouseActivity RETURNED false by pos !weMovedReticle");
        return false;
    }
    weMovedReticle = false;
    printd("ignoreMouseActivity RETURNED true by default (CHANGED weMovedReticle to false!)");
    return true;*/
}
printd("ignoreMouseActivity defined");
var MARGIN = 25;
var reticleMinX = MARGIN, reticleMaxX, reticleMinY = MARGIN, reticleMaxY;
function updateRecommendedArea() {
    var dims = Controller.getViewportDimensions();
    reticleMaxX = dims.x - MARGIN;
    reticleMaxY = dims.y - MARGIN;
}
var setReticlePosition = function (point2d) {
    weMovedReticle = true;
    point2d.x = Math.max(reticleMinX, Math.min(point2d.x, reticleMaxX));
    point2d.y = Math.max(reticleMinY, Math.min(point2d.y, reticleMaxY));
    /*if (Math.random()>0.75)
        printd('setReticlePosition ' + JSON.stringify(point2d));*/
    Reticle.setPosition(point2d);
    canClearReticle = false;
};

// Generalizations of utilities that work with system and overlay elements.
function findRayIntersection(pickRay) {
    // Check 3D overlays and entities. Argument is an object with origin and direction.
    var result = Overlays.findRayIntersection(pickRay);
    if (!result.intersects) {
        result = Entities.findRayIntersection(pickRay, true);
    }
    return result;
}
function isPointingAtOverlay(optionalHudPosition2d) {
    return true;
    var isPointing = Reticle.pointingAtSystemOverlay || Overlays.getOverlayAtPoint(optionalHudPosition2d || Reticle.position);
    printd("[CONTROLLER-2] isPointingAtOverlay handControllerPointer isPointing= " + isPointing);
    printd("[CONTROLLER-2] isPointingAtOverlay handControllerPointer Reticle.pointingAtSystemOverlay= " + Reticle.pointingAtSystemOverlay + " getOverlayAtPoint= " + Overlays.getOverlayAtPoint(optionalHudPosition2d || Reticle.position));
    printd("[CONTROLLER-2] isPointingAtOverlay handControllerPointer optionalHudPosition2d= " + optionalHudPosition2d + " Reticle.position= " + JSON.stringify(Reticle.position) + "  == ||= " + JSON.stringify(optionalHudPosition2d || Reticle.position));
    if (isPointing) {
            printd("[CONTROLLER-4] isPointingAtOverlay handControllerPointer " + JSON.stringify(optionalHudPosition2d || Reticle.position));// + isPointing + " Reticle.pointingAtSystemOverlay " + Reticle.pointingAtSystemOverlay + 
            //' optionalHudPosition2d ' + JSON.stringify(optionalHudPosition2d) + ' Reticle.position ' + JSON.stringify(Reticle.position) + 
            //' Overlays.getOverlayAtPoint(optionalHudPosition2d || Reticle.position) ' + Overlays.getOverlayAtPoint(optionalHudPosition2d || Reticle.position) );
    }
    return isPointing;
}

// Generalized HUD utilities, with or without HMD:
// This "var" is for documentation. Do not change the value!
var PLANAR_PERPENDICULAR_HUD_DISTANCE = 1;
function calculateRayUICollisionPoint(position, direction) {
    // Answer the 3D intersection of the HUD by the given ray, or falsey if no intersection.
    if (HMD.active) {
        return HMD.calculateRayUICollisionPoint(position, direction);
    }
    // interect HUD plane, 1m in front of camera, using formula:
    //   scale = hudNormal dot (hudPoint - position) / hudNormal dot direction
    //   intersection = postion + scale*direction
    var hudNormal = Quat.getFront(Camera.getOrientation());
    var hudPoint = Vec3.sum(Camera.getPosition(), hudNormal); // must also scale if PLANAR_PERPENDICULAR_HUD_DISTANCE!=1
    var denominator = Vec3.dot(hudNormal, direction);
    if (denominator === 0) {
        return null;
    } // parallel to plane
    var numerator = Vec3.dot(hudNormal, Vec3.subtract(hudPoint, position));
    var scale = numerator / denominator;
    return Vec3.sum(position, Vec3.multiply(scale, direction));
}
var DEGREES_TO_HALF_RADIANS = Math.PI / 360;
function overlayFromWorldPoint(point) {
    // Answer the 2d pixel-space location in the HUD that covers the given 3D point.
    // REQUIRES: that the 3d point be on the hud surface!
    // Note that this is based on the Camera, and doesn't know anything about any
    // ray that may or may not have been used to compute the point. E.g., the
    // overlay point is NOT the intersection of some non-camera ray with the HUD.
    if (HMD.active) {
        return HMD.overlayFromWorldPoint(point);
    }
    var cameraToPoint = Vec3.subtract(point, Camera.getPosition());
    var cameraX = Vec3.dot(cameraToPoint, Quat.getRight(Camera.getOrientation()));
    var cameraY = Vec3.dot(cameraToPoint, Quat.getUp(Camera.getOrientation()));
    var size = Controller.getViewportDimensions();
    var hudHeight = 2 * Math.tan(verticalFieldOfView * DEGREES_TO_HALF_RADIANS); // must adjust if PLANAR_PERPENDICULAR_HUD_DISTANCE!=1
    var hudWidth = hudHeight * size.x / size.y;
    var horizontalFraction = (cameraX / hudWidth + 0.5);
    var verticalFraction = 1 - (cameraY / hudHeight + 0.5);
    var horizontalPixels = size.x * horizontalFraction;
    var verticalPixels = size.y * verticalFraction;
    return { x: horizontalPixels, y: verticalPixels };
}

var gamePad = Controller.findDevice("GamePad");
function activeHudPoint2dGamePad() {
    if (!HMD.active) {
      return;
    }
    var headPosition = MyAvatar.getHeadPosition();
    var headDirection = Quat.getUp(Quat.multiply(MyAvatar.headOrientation, Quat.angleAxis(-90, { x: 1, y: 0, z: 0 })));

    var hudPoint3d = calculateRayUICollisionPoint(headPosition, headDirection);

    if (!hudPoint3d) {
        if (Menu.isOptionChecked("Overlays")) { // With our hud resetting strategy, hudPoint3d should be valid here
            printd('Controller is parallel to HUD');  // so let us know that our assumptions are wrong.
        }
        return;
    }
    var hudPoint2d = overlayFromWorldPoint(hudPoint3d);

    // We don't know yet if we'll want to make the cursor or laser visble, but we need to move it to see if
    // it's pointing at a QML tool (aka system overlay).
    setReticlePosition(hudPoint2d);

    return hudPoint2d;
}


function activeHudPoint2d(activeHand) { // if controller is valid, update reticle position and answer 2d point. Otherwise falsey.
    var shouldLog = activeTrigger.full() || Math.random()>0.5;
    //printd("activeHudPoint2d start activeHand " + activeHand);
    var controllerPose = getControllerWorldLocation(activeHand, true); // note: this will return head pose if hand pose is invalid (third eye)
    if (!controllerPose.valid) {
        if (shouldLog)
            printd('activeHudPoint2d invalid!');
        return; // Controller is cradled.
    }
    var controllerPosition = controllerPose.position;
    var controllerDirection = Quat.getUp(controllerPose.rotation);

    var hudPoint3d = calculateRayUICollisionPoint(controllerPosition, controllerDirection);
    if (!hudPoint3d) {
        if (Menu.isOptionChecked("Overlays")) { // With our hud resetting strategy, hudPoint3d should be valid here
            printd('Controller is parallel to HUD');  // so let us know that our assumptions are wrong.
        }
        if (shouldLog)
            printd('activeHudPoint2d BAD hudPoint3d');
        return;
    }
    printd('handControllerPointer hudPoint3d ' + hudPoint3d.x + ', ' + hudPoint3d.y + ', ' + hudPoint3d.z);
    var hudPoint2d = overlayFromWorldPoint(hudPoint3d);
    printd('handControllerPointer hudPoint2d ' + hudPoint2d.x + ', ' + hudPoint2d.y);

    // We don't know yet if we'll want to make the cursor or laser visble, but we need to move it to see if
    // it's pointing at a QML tool (aka system overlay).
    setReticlePosition(hudPoint2d);
    return hudPoint2d;
}

// MOUSE ACTIVITY --------
//
var isSeeking = false;
var averageMouseVelocity = 0, lastIntegration = 0, lastMouse;
var WEIGHTING = 1 / 20; // simple moving average over last 20 samples
var ONE_MINUS_WEIGHTING = 1 - WEIGHTING;
var AVERAGE_MOUSE_VELOCITY_FOR_SEEK_TO = 20;
function isShakingMouse() { // True if the person is waving the mouse around trying to find it.
    var now = Date.now(), mouse = Reticle.position, isShaking = false;
    if (lastIntegration && (lastIntegration !== now)) {
        var velocity = Vec3.length(Vec3.subtract(mouse, lastMouse)) / (now - lastIntegration);
        averageMouseVelocity = (ONE_MINUS_WEIGHTING * averageMouseVelocity) + (WEIGHTING * velocity);
        if (averageMouseVelocity > AVERAGE_MOUSE_VELOCITY_FOR_SEEK_TO) {
            isShaking = true;
        }
    }
    lastIntegration = now;
    lastMouse = mouse;
    return isShaking;
}
var NON_LINEAR_DIVISOR = 2;
var MINIMUM_SEEK_DISTANCE = 0.1;
function updateSeeking(doNotStartSeeking) {
    if (!doNotStartSeeking && (!Reticle.visible || isShakingMouse())) {
        if (!isSeeking) {
            printd('Start seeking mouse.');
            isSeeking = true;
        }
    } // e.g., if we're about to turn it on with first movement.
    if (!isSeeking) {
        return;
    }
    averageMouseVelocity = lastIntegration = 0;
    printd("updateSeeking, call getHUDLookAtPosition2D");
    var lookAt2D = HMD.getHUDLookAtPosition2D();
    if (!lookAt2D) { // If this happens, something has gone terribly wrong.
        printd('Cannot seek without lookAt position');
        isSeeking = false;
        return; // E.g., if parallel to location in HUD
    }
    var copy = Reticle.position;
    function updateDimension(axis) {
        var distanceBetween = lookAt2D[axis] - Reticle.position[axis];
        var move = distanceBetween / NON_LINEAR_DIVISOR;
        if (Math.abs(move) < MINIMUM_SEEK_DISTANCE) {
            return false;
        }
        copy[axis] += move;
        return true;
    }
    var okX = !updateDimension('x'), okY = !updateDimension('y'); // Evaluate both. Don't short-circuit.
    if (okX && okY) {
        printd('Finished seeking mouse');
        isSeeking = false;
    } else {
        Reticle.setPosition(copy); // Not setReticlePosition
        canClearReticle = false;
    }
}

var mouseCursorActivity = new TimeLock(5000);
var APPARENT_MAXIMUM_DEPTH = 100.0; // this is a depth at which things all seem sufficiently distant
function updateMouseActivity(isClick) {
    printd('updateMouseActivity calls ignoreMouseActivity()');
    if (ignoreMouseActivity()) {
        return;
    }
    var now = Date.now();
    mouseCursorActivity.update(now);
    if (isClick) {
        return;
    } // Bug: mouse clicks should keep going. Just not hand controller clicks
    printd('updateMouseActivity was not click, calling handControllerLockOut.update(now)');
    handControllerLockOut.update(now);
    Reticle.visible = true;
}
function expireMouseCursor(now) {
    if (!isPointingAtOverlay() && mouseCursorActivity.expired(now)) {
        Reticle.visible = false;
    }
}
function hudReticleDistance() { // 3d distance from camera to the reticle position on hud
    // (The camera is only in the center of the sphere on reset.)
    var reticlePositionOnHUD = HMD.worldPointFromOverlay(Reticle.position);
    return Vec3.distance(reticlePositionOnHUD, HMD.position);
}

function maybeAdjustReticleDepth() {
    if (HMD.active) { // set depth
        if (isPointingAtOverlay()) {
            Reticle.depth = hudReticleDistance();
        }
    }
}
var ADJUST_RETICLE_DEPTH_INTERVAL = 50; // 20hz
Script.setInterval(maybeAdjustReticleDepth,ADJUST_RETICLE_DEPTH_INTERVAL);

function onMouseMove() {
    // Display cursor at correct depth (as in depthReticle.js), and updateMouseActivity.
    printd('onMouseMove calls ignoreMouseActivity()');
    if (ignoreMouseActivity()) {
        return;
    }

    if (HMD.active) { // set depth
        updateSeeking();
        if (isPointingAtOverlay()) {
            Reticle.depth = hudReticleDistance();
        } else {
            var result = findRayIntersection(Camera.computePickRay(Reticle.position.x, Reticle.position.y));
            Reticle.depth = result.intersects ? result.distance : APPARENT_MAXIMUM_DEPTH;
        }
    }
    printd('onMouseMove calls updateMouseActivity()');
    updateMouseActivity(); // After the above, just in case the depth movement is awkward when becoming visible.
}
function onMouseClick() {
    updateMouseActivity(true);
}
setupHandler(Controller.mouseMoveEvent, onMouseMove);
setupHandler(Controller.mousePressEvent, onMouseClick);
setupHandler(Controller.mouseDoublePressEvent, onMouseClick);

// CONTROLLER MAPPING ---------
//

var leftTrigger = new Trigger('left');
var rightTrigger = new Trigger('right');
var activeTrigger = rightTrigger;
var activeHand = Controller.Standard.RightHand;
var LEFT_HUD_LASER = 1;
var RIGHT_HUD_LASER = 2;
var BOTH_HUD_LASERS = LEFT_HUD_LASER + RIGHT_HUD_LASER;
var activeHudLaser = RIGHT_HUD_LASER;
function toggleHand() { // unequivocally switch which hand controls mouse position
    printd("[CONTROLLER-3] toggleHand()");
    if (activeHand === Controller.Standard.RightHand) {
        activeHand = Controller.Standard.LeftHand;
        activeTrigger = leftTrigger;
        activeHudLaser = LEFT_HUD_LASER;
    } else {
        activeHand = Controller.Standard.RightHand;
        activeTrigger = rightTrigger;
        activeHudLaser = RIGHT_HUD_LASER;
    }
    clearSystemLaser();
}
function makeToggleAction(hand) { // return a function(0|1) that makes the specified hand control mouse when 1
    return function (on) {
        if (on && (activeHand !== hand)) {
            toggleHand();
        }
    };
}

var clickMapping = Controller.newMapping('handControllerPointer-click');
Script.scriptEnding.connect(clickMapping.disable);

function monitorRTClick(trigger) {
    return function() {
        if (trigger.full()) {
            printd('[DAYDREAM-CONTROLLER] monitorRTClick trigger ' + trigger.label);
        }
        return true;
    }
}

var prevWasFull = false;

// Gather the trigger data for smoothing.
clickMapping.from(Controller.Standard.RT).peek().to(rightTrigger.triggerPress);
clickMapping.from(Controller.Standard.LT).peek().to(leftTrigger.triggerPress);
clickMapping.from(Controller.Standard.RTClick).peek().when(monitorRTClick(rightTrigger)).to(rightTrigger.triggerClick);
clickMapping.from(Controller.Standard.LTClick).peek().to(leftTrigger.triggerClick);
// Full smoothed trigger is a click.
function isPointingAtOverlayStartedNonFullTrigger(trigger) {
    // true if isPointingAtOverlay AND we were NOT full triggered when we became so.
    // The idea is to not count clicks when we're full-triggering and reach the edge of a window.
    var lockedIn = false;
    return function (value) {
        //if (trigger.full()) {
            printd('[DAYDREAM-CONTROLLER] isPointingAtOverlayStartedNonFullTrigger analysis rightTrigger.full value {' + value + '} current trigger is full? ' + activeTrigger.full());
        //}
        if (trigger !== activeTrigger) {
//            if (lockedIn || Math.random()>0.98) {
                printd('[CONTROLLER-2] isPointingAtOverlayStartedNonFullTrigger (!== case) lockedIn ' + lockedIn + ' returning (trigger not active)'
                    + ' trigger: ' + trigger.label + '(' + trigger.full() + ') activeTrigger: ' + activeTrigger.label + '(' + activeTrigger.full() + ')');
//            }
            return lockedIn = false;
        }
        if (!isPointingAtOverlay()) {
            if (lockedIn) {
                printd('isPointingAtOverlayStartedNonFullTrigger lockedIn ' + lockedIn + ' returning (NOT POINTING AT OVERLAY)' + ' trigger: ' + trigger.label );
            }
            printd('isPointingAtOverlayStartedNonFullTrigger returning false (NOT POINTING AT OVERLAY)' + ' trigger: ' + trigger.label );
            return lockedIn = false;
        }
        if (lockedIn) {
            printd('[DAYDREAM-CONTROLLER] isPointingAtOverlayStartedNonFullTrigger returning TRUE lockedIn ' + lockedIn + ' returning' + ' trigger: ' + trigger.label );
            return true;
        }
        lockedIn = !trigger.full() && prevWasFull;
        if (lockedIn) {
            printd('isPointingAtOverlayStartedNonFullTrigger lockedIn (POINTING, LOCKED IN prev, is ActiveTrigger) ' + lockedIn + ' trigger: ' + trigger.label + '(' + trigger.full() + ')');
        }
        prevWasFull = trigger.full();
        printd('[DAYDREAM-CONTROLLER] isPointingAtOverlayStartedNonFullTrigger RETURN lockedIn ' + lockedIn);
        return lockedIn;
    }
}
printd("isPointingAtOverlayStartedNonFullTrigger defined");
//clickMapping.from(rightTrigger.full).when(isPointingAtOverlayStartedNonFullTrigger(rightTrigger)).to(Controller.Actions.ReticleClick); before
//clickMapping.from(leftTrigger.full).when(isPointingAtOverlayStartedNonFullTrigger(leftTrigger)).to(Controller.Actions.ReticleClick); before
clickMapping.from(rightTrigger.full).debug().when(isPointingAtOverlayStartedNonFullTrigger(rightTrigger)).to(Controller.Actions.ReticleClick);
/*clickMapping.from(leftTrigger.full).when(isPointingAtOverlayStartedNonFullTrigger(leftTrigger)).to(function (clicked) {
    if (clicked || Math.random()>0.98) {
        printd("leftTrigger full? " + clicked );
    }
});*/

// The following is essentially like Left and Right versions of
// clickMapping.from(Controller.Standard.RightSecondaryThumb).peek().to(Controller.Actions.ContextMenu);
// except that we first update the reticle position from the appropriate hand position, before invoking the  .
var wantsMenu = 0;
clickMapping.from(function () { return wantsMenu; }).to(Controller.Actions.ContextMenu);
clickMapping.from(Controller.Standard.RightSecondaryThumb).peek().to(function (clicked) {
    if (clicked) {
        activeHudPoint2d(Controller.Standard.RightHand);
    }
    wantsMenu = clicked;
});
clickMapping.from(Controller.Standard.LeftSecondaryThumb).peek().to(function (clicked) {
    if (clicked) {
        activeHudPoint2d(Controller.Standard.LeftHand);
    }
    wantsMenu = clicked;
});
clickMapping.from(Controller.Standard.Start).peek().to(function (clicked) {
    if (clicked) {
        activeHudPoint2dGamePad();
      }

      wantsMenu = clicked;
});
/* // anddb- TODO Keyboard must work in all platforms except mobile, restore it and find a way to disable it (e.g. for android) */
/*clickMapping.from(Controller.Hardware.Keyboard.RightMouseClicked).peek().to(function () {
    // Allow the reticle depth to be set correctly:
    // Wait a tick for the context menu to be displayed, and then simulate a (non-hand-controller) mouse move
    // so that the system updates qml state (Reticle.pointingAtSystemOverlay) before it gives us a mouseMove.
    // We don't want the system code to always do this for us, because, e.g., we do not want to get a mouseMove
    // after the Left/RightSecondaryThumb gives us a context menu. Only from the mouse.
    Script.setTimeout(function () {
        Reticle.setPosition(Reticle.position);
    }, 0);
});*/
// Partial smoothed trigger is activation.
//clickMapping.from(rightTrigger.partial).to(makeToggleAction(Controller.Standard.RightHand)); before
//clickMapping.from(leftTrigger.partial).to(makeToggleAction(Controller.Standard.LeftHand)); before
clickMapping.from(rightTrigger.full).to(makeToggleAction(Controller.Standard.RightHand));
clickMapping.from(leftTrigger.full).to(makeToggleAction(Controller.Standard.LeftHand));
clickMapping.enable();

printd("clickMapping defined");
// VISUAL AID -----------
// Same properties as handControllerGrab search sphere
var LASER_ALPHA = 0.5;
var LASER_SEARCH_COLOR_XYZW = {x: 10 / 255, y: 10 / 255, z: 255 / 255, w: LASER_ALPHA};
var LASER_TRIGGER_COLOR_XYZW = {x: 250 / 255, y: 10 / 255, z: 10 / 255, w: LASER_ALPHA};
var SYSTEM_LASER_DIRECTION = {x: 0, y: 0, z: -1};
var systemLaserOn = false;

function clearSystemLaser() {
    // clear the reticle position with a delay so the position is still there when Application gets the event
    clearReticlePositionDelayed();
    return;
    // For android the laser is always ON
    /*printd("[CONTROLLER-3] clearSystemLaser systemLaserOn " + systemLaserOn);
    if (!systemLaserOn) {
        return;
    }
    HMD.disableHandLasers(BOTH_HUD_LASERS);
    HMD.disableExtraLaser();
    systemLaserOn = false;
    weMovedReticle = true;
    Reticle.position = { x: -1, y: -1};*/
}
function setColoredLaser() { // answer trigger state if lasers supported, else falsey.
    printd("[CONTROLLER-4] [LASERS] setColoredLaser trigger " + activeTrigger.state + " isHandControllerAvailable " + HMD.isHandControllerAvailable());
    var color = (activeTrigger.state === 'full') ? LASER_TRIGGER_COLOR_XYZW : LASER_SEARCH_COLOR_XYZW;

    if (!HMD.isHandControllerAvailable()) {
        printd("[CONTROLLER-4] [LASERS] isHandControllerAvailable was false");
        // NOTE: keep this offset in sync with scripts/system/librarires/controllers.js:57
        var VERTICAL_HEAD_LASER_OFFSET = 0.1;
        var position = Vec3.sum(HMD.position, Vec3.multiplyQbyV(HMD.orientation, {x: 0, y: VERTICAL_HEAD_LASER_OFFSET, z: 0}));
        var orientation = Quat.multiply(HMD.orientation, Quat.angleAxis(-90, { x: 1, y: 0, z: 0 }));
        return HMD.setExtraLaser(position, true, color, Quat.getUp(orientation));
    }

    return HMD.setHandLasers(activeHudLaser, true, color, SYSTEM_LASER_DIRECTION) && activeTrigger.state;
}

printd(" setColoredLaser defined");
// MAIN OPERATIONS -----------
//
function update() {
    // laser always on (changes color if clicked)
    if (/*!systemLaserOn || */(systemLaserOn !== activeTrigger.state)) {
        // ^ avoid calling setColoredLaser everytime when not pressed
        systemLaserOn = setColoredLaser();
        printd("[CONTROLLER-4] setColoredLaser result " + systemLaserOn);
    } else {
        printd("[CONTROLLER-4] setColoredLaser state is still.. " + activeTrigger.state + " and systemLaserOn was " + systemLaserOn);
    }

    var shouldLog = Math.random()>0.98;
    var now = Date.now();
    function off() {
        printd("[CONTROLLER-3] off: expireMouseCursor and clearSystemLaser");
        expireMouseCursor();
        clearSystemLaser();
    }

    updateSeeking(true);
    if (!handControllerLockOut.expired(now)) {
        printd("[CONTROLLER-3] calling off // Let them use mouse in peace.");
        return off(); // Let them use mouse in peace.
    }

    if (!Menu.isOptionChecked("First Person")) {
        printd("[CONTROLLER-3] calling off // !Menu.isOptionChecked(First Person)");
        return off(); // What to do? menus can be behind hand!
    }

    if ((!Window.hasFocus() && !HMD.active) || !Reticle.allowMouseCapture) {
        // In desktop it's pretty clear when another app is on top. In that case we bail, because
        // hand controllers might be sputtering "valid" data and that will keep someone from deliberately
        // using the mouse on another app. (Fogbugz case 546.)
        // However, in HMD, you might not realize you're not on top, and you wouldn't be able to operate
        // other apps anyway. So in that case, we DO keep going even though we're not on top. (Fogbugz 1831.)
        printd("[CONTROLLER-3] calling off // Don't mess with other apps or paused mouse activity");
        return off(); // Don't mess with other apps or paused mouse activity
    }

    leftTrigger.update();
    rightTrigger.update();

    if (!activeTrigger.state) {
        printd("[CONTROLLER-3] calling off // No trigger");
        return off(); // No trigger
    }

    if (getGrabCommunications()) {
        printd("[CONTROLLER-3] calling off // getGrabCommunications");
        return off();
    }
    var hudPoint2d;
    if (activeHand) {
        hudPoint2d = activeHudPoint2d(activeHand);
    }
    if (!hudPoint2d) {
        printd("[CONTROLLER-3] calling off // !hudPoint2d");        
        return off();
    }

    if (shouldLog)
        printd("[CONTROLLER-4]-handControllerPointer.js hudPoint2d is at: " + JSON.stringify(hudPoint2d));

    // If there's a HUD element at the (newly moved) reticle, just make it visible and bail.
    /*if (isPointingAtOverlay(hudPoint2d)) {
        if (HMD.active) {
            Reticle.depth = hudReticleDistance();

            if (!HMD.isHandControllerAvailable()) {

                var color = (activeTrigger.state === 'full') ? LASER_TRIGGER_COLOR_XYZW : LASER_SEARCH_COLOR_XYZW;
                var position = MyAvatar.getHeadPosition();
                var direction = Quat.getUp(Quat.multiply(MyAvatar.headOrientation, Quat.angleAxis(-90, { x: 1, y: 0, z: 0 })));
                HMD.setExtraLaser(position, true, color, direction);
            }
        }

        if (activeTrigger.state && (!systemLaserOn || (systemLaserOn !== activeTrigger.state))) { // last=>wrong color
            // If the active plugin doesn't implement hand lasers, show the mouse reticle instead.
            systemLaserOn = setColoredLaser();
            Reticle.visible = true; // !systemLaserOn;
        } else if ((systemLaserOn || Reticle.visible) && !activeTrigger.state) {
            printd("[CONTREOLLER-3] isPointingAtOverlay " + JSON.stringify(hudPoint2d) + " (systemLaserOn " + systemLaserOn + " || Reticle.visible " + Reticle.visible + ") && (!) activeTrigger.state " + activeTrigger.state);
            //clearSystemLaser();
            //Reticle.visible = true;
        }
        return;
    }
    // We are not pointing at a HUD element (but it could be a 3d overlay).
    printd("[CONTROLLER-3] We are not pointing at a HUD element (but it could be a 3d overlay).");
    //clearSystemLaser();
    Reticle.visible = true; // false;*/
}

printd("update defined");
var BASIC_TIMER_INTERVAL = 20; // 20ms = 50hz good enough
var updateIntervalTimer = Script.setInterval(function(){
    update();
}, BASIC_TIMER_INTERVAL);

printd("updateIntervalTimer defined");

// Check periodically for changes to setup.
var SETTINGS_CHANGE_RECHECK_INTERVAL = 10 * 1000; // 10 seconds
function checkSettings() {
    updateFieldOfView();
    updateRecommendedArea();
}
checkSettings();

var CLEAR_RETICLE_TIMEOUT = BASIC_TIMER_INTERVAL * 10;
// remove var WE_MOVED_RETICLE_PREVENT_TIMEOUT = CLEAR_RETICLE_TIMEOUT / 2;
var canClearReticle = false; // false when a position was already set, true after a clearReticle order was issued
function clearReticlePositionDelayed() {
    canClearReticle = true;
    weMovedReticle = true;
    Script.setTimeout(function() {
        if (canClearReticle) {
            weMovedReticle = true; // new to avoid problem of not ignoring mouse and cancelling update()
            Reticle.position = { x: -1, y: -1};
            printd("clearReticle done");
        } else {
            printd("clearReticle cancelled");
        }
    }, CLEAR_RETICLE_TIMEOUT);
}

printd("checkSettings called");
var settingsChecker = Script.setInterval(checkSettings, SETTINGS_CHANGE_RECHECK_INTERVAL);
Script.scriptEnding.connect(function () {
    Script.clearInterval(settingsChecker);
    Script.clearInterval(updateIntervalTimer);
    OffscreenFlags.navigationFocusDisabled = false;
});

printd("end");
}()); // END LOCAL_SCOPE
