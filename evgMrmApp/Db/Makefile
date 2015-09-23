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
DB += evg-vme-300.db
DB += evg-vme.db
DB += evg-cpci.db 

## Install non form factor specific tempalates
DB += evgSoftSeq.db

# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE
