add_library(dtnd_lib STATIC
    CapsuleWorker.cpp
    Component.cpp
    Configuration.cpp
    Debugger.cpp
    DevNull.cpp
    DTNTPWorker.cpp
    EchoWorker.cpp
    NativeDaemon.cpp
    api/ApiP2PExtensionHandler.cpp
    api/ApiServer.cpp
    api/BinaryStreamClient.cpp
    api/BundleStreamBuf.cpp
    api/ClientHandler.cpp
    api/EventConnection.cpp
    api/ExtendedApiHandler.cpp
    api/ManagementConnection.cpp
    api/NativeSerializerCallback.cpp
    api/NativeSerializer.cpp
    api/NativeSession.cpp
    api/OrderedStreamHandler.cpp
    api/Registration.cpp
    core/AbstractWorker.cpp
    core/BundleCore.cpp
    core/BundleEvent.cpp
    core/BundleExpiredEvent.cpp
    core/BundleFilter.cpp
    core/BundleFilterTable.cpp
    core/BundlePurgeEvent.cpp
    core/CustodyEvent.cpp
    core/Event.cpp
    core/EventReceiver.cpp
    core/EventSwitch.cpp
    core/FragmentManager.cpp
    core/GlobalEvent.cpp
    core/Node.cpp
    core/NodeEvent.cpp
    core/StatusReportGenerator.cpp
    core/TimeAdjustmentEvent.cpp
    core/TimeEvent.cpp
    core/WallClock.cpp
    core/filter/LogFilter.cpp
    core/filter/SecurityFilter.cpp
    net/BundleTransfer.cpp
    net/ConnectionEvent.cpp
    net/ConnectionManager.cpp
    net/ConvergenceLayer.cpp
    net/DatagramConnection.cpp
    net/DatagramConvergenceLayer.cpp
    net/DatagramService.cpp
    net/DiscoveryAgent.cpp
    net/DiscoveryBeacon.cpp
    net/DiscoveryBeaconHandler.cpp
    net/DiscoveryService.cpp
    net/FileConvergenceLayer.cpp
    net/FileMonitor.cpp
    net/IPNDAgent.cpp
    net/P2PDialupEvent.cpp
    net/P2PDialupExtension.cpp
    net/TCPConnection.cpp
    net/TCPConvergenceLayer.cpp
    net/TransferAbortedEvent.cpp
    net/TransferCompletedEvent.cpp
    net/UDPConvergenceLayer.cpp
    net/UDPDatagramService.cpp
    routing/BaseRouter.cpp
    routing/NeighborDatabase.cpp
    routing/NeighborDataset.cpp
    routing/NeighborRoutingExtension.cpp
    routing/NodeHandshake.cpp
    routing/NodeHandshakeEvent.cpp
    routing/NodeHandshakeExtension.cpp
    routing/QueueBundleEvent.cpp
    routing/RequeueBundleEvent.cpp
    routing/RetransmissionExtension.cpp
    routing/RoutingExtension.cpp
    routing/SchedulingBundleIndex.cpp
    routing/StaticRouteChangeEvent.cpp
    routing/StaticRoute.cpp
    routing/StaticRoutingExtension.cpp
    routing/epidemic/EpidemicRoutingExtension.cpp
    routing/flooding/FloodRoutingExtension.cpp
    storage/BundleIndex.cpp
    storage/BundleResult.cpp
    storage/BundleStorage.cpp
    storage/DataStorage.cpp
    storage/MemoryBundleStorage.cpp
    storage/MetaStorage.cpp
    storage/SimpleBundleStorage.cpp
)

if (DtnSec)
  target_sources(dtnd_lib
    PRIVATE
      security/SecurityKeyManager.cpp
      security/SecurityManager.cpp
      security/exchange/DHProtocol.cpp
      security/exchange/HashProtocol.cpp
      security/exchange/KeyExchangeData.cpp
      security/exchange/KeyExchangeEvent.cpp
      security/exchange/KeyExchangeProtocol.cpp
      security/exchange/KeyExchanger.cpp
      security/exchange/KeyExchangeSession.cpp
      security/exchange/NFCProtocol.cpp
      security/exchange/NoneProtocol.cpp
      security/exchange/QRCodeProtocol.cpp
  )
endif()

if (EnableTls)
    target_sources(dtnd_lib PRIVATE security/SecurityCertificateManager.cpp)
endif()

if (Lowpan)
  target_sources(dtnd_lib
    PRIVATE
      net/LOWPANConnection.cpp
      net/LOWPANConvergenceLayer.cpp
      net/LOWPANDatagramService.cpp
  )
endif()

if (HAVE_REGEX_H)
  target_sources(dtnd_lib PRIVATE routing/StaticRegexRoute.cpp)
endif()

# TODO: set up CURL
#    net/HTTPConvergenceLayer.cpp

# TODO: set up VMIME
#    net/EMailConvergenceLayer.cpp
#    net/EMailImapService.cpp
#    net/EMailSmtpService.cpp

# TODO: set up wifip2p
#    net/WifiP2PManager.cpp
# This file is never linked, class never used: net/NativeP2pManager.cpp

# TODO: set up sqlite
#    storage/SQLiteBundleSet.cpp
#    storage/SQLiteBundleStorage.cpp
#    storage/SQLiteConfigure.cpp
#    storage/SQLiteDatabase.cpp

# TODO: set up HAVE_LIBDAEMON, HAVE_DAEMON_RESET_SIGS

# TODO: add configration option, test it builds.
if (DtnDht)
  target_sources(dtnd_lib PUBLIC net/DHTNameService.cpp)
  add_definitions(-DWITH_DHT_NAMESERVICE=1)
endif()

target_include_directories(dtnd_lib PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(dtnd_lib ibrcommon ibrdtn)

add_executable(dtnd "Main.cpp")
target_link_libraries(dtnd dtnd_lib)

install(TARGETS dtnd RUNTIME DESTINATION sbin)
