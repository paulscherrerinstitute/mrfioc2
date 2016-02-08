TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Install databases, templates & substitutions like this
# DB += dbExample1.db
# DB += dbExample2.db
# DB += dbSubExample.db
# DB += user.substitutions

##Generate form factor specific database files
DB += evr-cpci-230.db
DB += evr-pcie-300.db
DB += evr-vme-300.db
DB += evr-vme-230.db
# DB += evr-pmc-230.db


## Install non form factor specific tempalates
DB += evr-softEvent.template
DB += evr-softEvent-measure.template
DB += evr-specialFunctionMap.template
DB += evr-pulserMap.template
DB += evr-pulserMap-dbus.template
DB += evr-health.template
DB += evr-delayModule.template
DB += evr-health.template


# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE
