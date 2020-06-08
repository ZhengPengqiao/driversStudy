# readme

## clk_1_of

* 针对设备树中的每一个isi设备(fsl,imx8-isi-ts)，都重新加载驱动获取相应isi设备的clk信息。

## clk_4_of

* 匹配到设备树中的mxc-md-ts节点(fsl,mxc-md-ts)，然后代码中遍历所有子节点,获取到所有isi设备的clk信息。

## clk_4_of_node

* 匹配到设备树中的mxc-md-ts节点(fsl,mxc-md-ts)，将所有节点的clk信息，都放在fsl,mxc-md-ts节点中，代码中按照标号获取所有clk信息。

