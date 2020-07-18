#!/bin/bash
DIR="`pwd`"
open ./wizard/wizard.app --args update "${DIR}/antarctica_pyramids"
sleep 3
open ./wizard/wizard.app --args update "${DIR}/filetest"
sleep 3
open ./wizard/wizard.app --args update "${DIR}/tests"
sleep 3
open ./wizard/wizard.app --args update "${DIR}/wizard"
