/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include "clk_4_of.h"

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
	int retval = 0;
	struct device_node *node;
	int isi_index = 0;

	dev_info(&pdev->dev, "In %s() (COMPILE_TIME=%s KO_VERSION=%s)\n", __func__, COMPILE_TIME, KO_VERSION);

	memset(&pxd, 0, sizeof(struct clk_test));
	pxd.pdev = pdev;

	for_each_available_child_of_node(dev->of_node, node)
	{

		dev_info(dev, "%s() node->name=%s\n", __func__, node->name);

		if (!strcmp(node->name, "isi"))
		{

			pxd.mxc_isi[isi_index] = devm_kzalloc(dev, sizeof(struct mxc_isi_dev), GFP_KERNEL);

			if (!pxd.mxc_isi[isi_index])
			{
				dev_info(dev, "%s() mxc_isi is null\n", __func__);
				return -ENOMEM;
			}

			pxd.mxc_isi[isi_index]->id = of_alias_get_id(node, "isi");
			dev_info(dev, "%s, isi_index=%d isi_%d\n", __func__, isi_index, pxd.mxc_isi[isi_index]->id);

			pxd.mxc_isi[isi_index]->clk = of_clk_get(node, 0);
			if (IS_ERR(pxd.mxc_isi[isi_index]->clk))
			{
				dev_err(dev, "isi_index=%d failed to get isi clk\n", isi_index);
				return PTR_ERR(pxd.mxc_isi[isi_index]->clk);
			}
			else
			{
				dev_info(dev, "of_clk_get pxd.mxc_isi[%d]->clk=%s ok\n", isi_index, clk_get_name(pxd.mxc_isi[isi_index]->clk));
			}

			retval = clk_prepare(pxd.mxc_isi[isi_index]->clk);
			if (retval < 0) {
				dev_err(dev, "%s, clk_prepare %s clk error retval=%d\n", __func__, clk_get_name(pxd.mxc_isi[isi_index]->clk), retval);
			}
			else
			{
				dev_info(dev, "%s, clk_prepare %s clk Ok\n", __func__,clk_get_name(pxd.mxc_isi[isi_index]->clk));
			}

			retval = clk_enable(pxd.mxc_isi[isi_index]->clk);
			if (retval < 0) {
				dev_err(dev, "%s, clk_enable %s clk error retval=%d\n", __func__, clk_get_name(pxd.mxc_isi[isi_index]->clk), retval);
			}
			else
			{
				dev_info(dev, "%s, clk_enable %s clk Ok\n", __func__,clk_get_name(pxd.mxc_isi[isi_index]->clk));
			}


			dev_info(dev, "isi_index=%d ok, mxc_isi.%d registered successfully isi_num=%d\n", isi_index, pxd.mxc_isi[isi_index]->id, pxd.isi_num);
			pxd.isi_num++;
			isi_index++;
		}
	}

	platform_set_drvdata(pdev, &pxd);

	return 0;
}

static int mxc_isi_remove(struct platform_device *pdev)
{
	int i = 0;
	struct clk_test *pxd_p = platform_get_drvdata(pdev);

	dev_info(&pxd_p->pdev->dev, "In %s()\n", __func__);
 
	for ( i = 0; i < pxd_p->isi_num; i++ )
	{
		pr_info("in %s() mxc_isi[%d]=%d clk_name=%s\n", __func__, i, pxd_p->mxc_isi[i]->id, clk_get_name(pxd_p->mxc_isi[i]->clk));
		if( pxd_p->mxc_isi[i] != NULL && pxd_p->mxc_isi[i]->clk != NULL )
		{
			clk_disable_unprepare(pxd_p->mxc_isi[i]->clk);
		}
		else
		{
			pr_err("in %s() mxc_isi != NULL && mxc_isi->clk != NULL\n", __func__);
		}

	}
	return 0;
}

static const struct of_device_id mxc_isi_of_match[] = {
	{.compatible = "fsl,mxc-md-ts",},
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
