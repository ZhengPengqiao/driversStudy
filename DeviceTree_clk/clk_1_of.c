/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include "clk_1_of.h"
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/delay.h>

struct clk_test pxd;

/**
 * @brief 获取clk结构体中的name成员
 * (struct clk *) -> (struct clk_core *) -> (const char *name)
 * 
 * @param clk : 需要获取名字的clk的指针
 * @return char* : name字符串的指针
 */
char *clk_get_name(struct clk *clk)
{
	return (*(*(const char ***)clk));
}


static int mxc_isi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mxc_isi_dev *mxc_isi;
	int retval = 0;
	dev_info(&pdev->dev, "In %s() (COMPILE_TIME=%s KO_VERSION=%s)\n", __func__, COMPILE_TIME, KO_VERSION);

	mxc_isi = devm_kzalloc(dev, sizeof(*mxc_isi), GFP_KERNEL);
	if (!mxc_isi)
		return -ENOMEM;

	mxc_isi->pdev = pdev;
	mxc_isi->id = of_alias_get_id(mxc_isi->pdev->dev.of_node, "isi");
	mxc_isi->clk = of_clk_get(mxc_isi->pdev->dev.of_node, 0);
	if (IS_ERR(mxc_isi->clk))
	{
		dev_err(dev, "mxc_isi->id=%d failed to get isi clk\n", mxc_isi->id);
		return PTR_ERR(mxc_isi->clk);
	}
	else
	{
		dev_info(dev, "of_clk_get mxc_isi[%d]->clk=%s ok\n", mxc_isi->id, clk_get_name(mxc_isi->clk));
	}

	retval = clk_prepare(mxc_isi->clk);
	if (retval < 0) {
		dev_err(dev, "%s, clk_prepare %s clk error retval=%d\n", __func__, clk_get_name(mxc_isi->clk), retval);
	}
	else
	{
		dev_info(dev, "%s, clk_prepare %s clk Ok\n", __func__,clk_get_name(mxc_isi->clk));
	}

	retval = clk_enable(mxc_isi->clk);
	if (retval < 0) {
		dev_err(dev, "%s, clk_enable %s clk error retval=%d\n", __func__, clk_get_name(mxc_isi->clk), retval);
	}
	else
	{
		dev_info(dev, "%s, clk_enable %s clk Ok\n", __func__,clk_get_name(mxc_isi->clk));
	}

	platform_set_drvdata(pdev, mxc_isi);

	pxd.isi_num++;
	pxd.mxc_isi[mxc_isi->id]= mxc_isi;
	dev_info(dev, "mxc_isi.%d registered successfully isi_num=%d\n", mxc_isi->id, pxd.isi_num);

	return 0;
}


static int mxc_isi_remove(struct platform_device *pdev)
{
	struct mxc_isi_dev *mxc_isi = platform_get_drvdata(pdev);
	pr_info("in %s() mxc_isi->id=%d clk_name=%s\n", __func__, mxc_isi->id, clk_get_name(mxc_isi->clk));
	if( mxc_isi != NULL && mxc_isi->clk != NULL )
	{
		clk_disable_unprepare(mxc_isi->clk);
	}
	else
	{
		pr_err("in %s() mxc_isi != NULL && mxc_isi->clk != NULL\n", __func__);
	}
	
	return 0;
}

static const struct of_device_id mxc_isi_of_match[] = {
	{.compatible = "fsl,imx8-isi-ts",},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mxc_isi_of_match);

static struct platform_driver mxc_isi_driver = {
	.probe		= mxc_isi_probe,
	.remove		= mxc_isi_remove,
	.driver = {
		.of_match_table = mxc_isi_of_match,
		.name		= MXC_ISI_DRIVER_NAME,
	}
};

module_platform_driver(mxc_isi_driver);

MODULE_LICENSE("GPL");
