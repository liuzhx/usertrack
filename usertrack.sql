CREATE TABLE `UserInfo_4` (
  `SwAddr` varchar(15) DEFAULT NULL,
  `Vlan` varchar(4) DEFAULT NULL,
  `MACAddr` varchar(20) DEFAULT NULL,
  `IfName` varchar(40) DEFAULT NULL,
  `IfIndex` int(6) DEFAULT NULL,
  `UserAddr` varchar(15) DEFAULT NULL,
  `Time` datetime DEFAULT NULL,
  KEY `SwAddr` (`SwAddr`),
  KEY `Vlan` (`Vlan`),
  KEY `MACAddr` (`MACAddr`),
  KEY `UserAddr` (`UserAddr`),
  KEY `Time` (`Time`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8

CREATE TABLE `UserInfo_6` (
  `SwAddr` varchar(15) DEFAULT NULL,
  `Vlan` varchar(4) DEFAULT NULL,
  `MACAddr` varchar(20) DEFAULT NULL,
  `IfName` varchar(20) DEFAULT NULL,
  `IfIndex` int(6) DEFAULT NULL,
  `UserAddr6` varchar(40) DEFAULT NULL,
  `Time` datetime DEFAULT NULL,
  KEY `UserAddr6` (`UserAddr6`),
  KEY `SwAddr` (`SwAddr`),
  KEY `Vlan` (`Vlan`),
  KEY `MACAddr` (`MACAddr`),
  KEY `Time` (`Time`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8



