# Changelog

All notable changes to this RDK Service will be documented in this file.

* Each RDK Service has a CHANGELOG file that contains all changes done so far. When version is updated, add a entry in the CHANGELOG.md at the top with user friendly information on what was changed with the new version. Please don't mention JIRA tickets in CHANGELOG. 

* Please Add entry in the CHANGELOG for each version change and indicate the type of change with these labels:
    * **Added** for new features.
    * **Changed** for changes in existing functionality.
    * **Deprecated** for soon-to-be removed features.
    * **Removed** for now removed features.
    * **Fixed** for any bug fixes.
    * **Security** in case of vulnerabilities.

* Changes in CHANGELOG should be updated when commits are added to the main or release branches. There should be one CHANGELOG entry per JIRA Ticket. This is not enforced on sprint branches since there could be multiple changes for the same JIRA ticket during development. 

* For more details, refer to [versioning](https://github.com/rdkcentral/rdkservices#versioning) section under Main README.

## [1.0.9] - 2023-09-21
### Fixed
- Fixed Update wifi state cache if wifi interface was enabled and added persist param in wifi setEnabled API to persist wifi state or not persist wifi state after reboot

## [1.0.8] - 2023-09-12
### Added
- Implement Thunder Plugin Configuration for Kirkstone builds(CMake-3.20 & above)

## [1.0.7] - 2023-05-22
### Fixed
- Fixed Update wifi state cache if wifi interface was disabled

## [1.0.6] - 2023-04-12
### Fixed
- Fixed IARM_Bus_UnRegisterEventHandler  call to IARM_Bus_RemoveEventHandler

## [1.0.5] - 2023-02-16
### Fixed
- Fix for WPEFramework crash with callstack std::execute

## [1.0.4] - 2022-12-13
### Added
- Added WifiManager plugin unitTest cases.

## [1.0.3] - 2022-11-17
### Fixed
- Avoid throw invalid argument error

## [1.0.2] - 2022-10-18
### Changed
- API access on all versions of the handler

## [1.0.2] - 2022-10-14
### Changed
-  Change the GetConnectedSSID to call netsrvmgr

## [1.0.1] - 2022-10-04
### Fixed
- Fixed warnings that are treated as errors with "-Wall -Werror"

## [1.0.0] - 2022-05-11
### Added
- Add CHANGELOG

### Change
- Reset API version to 1.0.0
- Change README to inform how to update changelog and API version
