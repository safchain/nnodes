<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:template match="/nnodes">
		<HTML>
			<HEAD>
				<TITLE>Nnodes Records Status</TITLE>
				<STYLE>
				body {
					background-color: #000;
					color: #eee;
					margin: 20px;
					font-size: 18px;
				}
				.logo {
					width: 128px;
					height: 128px;
					background-repeat: no-repeat;
					background-image: url(data:image/jpeg;base64,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);
					margin: 5px;
					float:left;
				}
				h1, h2, h3 {
					margin: 0px;
				}
				.record {
					background-color: #111;
					margin: 10px;
					padding: 5px;
					-webkit-border-radius: 10px;
					-moz-border-radius: 10px;
					border-radius: 10px;
				}
				table {
					width: 100%;
				}
				.ok {
					width: 22px;
					height: 22px;
					background-repeat: no-repeat;
					background-image: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABYAAAAWCAYAAADEtGw7AAAABHNCSVQICAgIfAhkiAAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAAOJSURBVDiNtdVvSJ1VHAfw73nOeZ7n+lx1S521WDfG1p83yx5NGKnEhGi0QWAvotXyVpARvSkYe+O2oBE0VhPCmkHlNhejBLWRGFmsCXcirkubZnFrw+lV7+xeb8/13ufPfZ7768V2RZlusvAH5805fD/ncDj8DiMirEVJa6ICEP8nzF6tVBqq6j+6Rysu/WKg+0366ldjYZGI7mogqMv7v2kJO65FXs6lY32nphDUixfWlwkEENR9d0Lf+fro8LydJDNrkJk1yHJTdLinbQJBvegWuP7DN9qGroSpI9RjIqhvWwltaj88ODs/Sf+kxylhTlDSmqSZ+d/owlQLPdfy9h8I6jLLv4q6D14//vGet5qyuQwUoWJsctp5pe29ajoRvrToTsXeml3n3n/+pRrGHHBJApcYnJyBsbkTYDwNL12HZ/cf2yMBQOWhF1sPNextis1PYM5MIJaaxqayAqW1cd8Qa9QrbqJywxP1/ft27a5JZ2OwPAO2Z8BworgcbwekNMgrQlfoLwtD0euMv1a1u/GpurN+LY3qwHaAAVxikDmHJmtIGC4NXxmPPLihJFC99QGfIluQhQSZcxAsXE31gfMMGBXizI8+q+Po6WZE4meEkERZykoiPBNCLDWDJzfXwifLkIUE2zOgapztqCh5WBEMdm4a5HHkIOFfZxazVhiKnIXn+dHemzO7Pzl9AJH4t0QU5d5j5aOJuL0jUH5vIJqK4Fryb3CJoCkqCA6IHBBzQHAAloXlzeG6OYKEPQZVBjy3AJ91W2bv8e/z6DUAYEQEpm/USqq2/PDoI75a2TcDTRVYr/lRXlSKEv86aLIAFx6YZIJzG5oq4FcFXFfGp12ZzIVTAwcXowswADB9o7Ze39xXua2wrmhdfCGsqQJ+RUBT5SVzOU/B5z1mJrQMCizqFRSeziTDV3deHrMGuLsJqpCgCH5jyByKkG7OSXBdFSd7c+ZK6BI4j8+G/nxmMJw6X0Bbl2D5TZDT0HVONX/+sv8AIvHO5dBbYACgkZg5NTC6s39wLlSmVCw6tQSFF+P8cLl1tvW7PDq+UoNatm3SSMyM/HLx6c6fokOBwu1QhIRiZQMujW6x24+cbL4TegO5XbN5/D6t9uDLw72/d1Bz5xEbD5W+CyCwmu638CpWKsaYgqr7X0AknoJhXySiidsG8rnVfE2MMQZAEFF2Neiq4bupNfvz/gNzCVIpeQ+buQAAAABJRU5ErkJggg==);
					float:left;
					margin-right: 10px;
				}
				.ko {
					width: 22px;
					height: 22px;
					background-repeat: no-repeat;
					background-image: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABYAAAAWCAYAAADEtGw7AAAABHNCSVQICAgIfAhkiAAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAATkSURBVDiNdZVrbBRVGIbfs7O73UtbWoqtlMRuaQmXFlBoAdG0CdEEVBLlh0a8VYlF1IQfBAgSQyLQItAfopFIFEgUI0EBa2lSaeSipFAsIFRoY7G0tt3Sy7rTnZ05M3Mu/tjuslvwJG8mM+fLM+d7z/edQ6SUmDgaVlatZZFo//NnjzXcNzk+Dk8r83hyst9hhtH86l+/td8XIKVM0eUd+35gui6t8Jg88dr7NRPnpZQ4lL/Q88enhzr07h45eOkq+7ykYtnEmJSXS9vqjtlqRJr9QUnv9MpoX1B+80JV7QSot233/ptq6xXZX/eZHDx4RN48eNTaV7wkBU7iVrR8sOu7BRvWvgRdh9ANCMOA5Bx2mgf1m7fXvHHqyNbD08q8c95efbmocnEJvdwGSSmi127A5hJ00WLrwpdfL1/f3XoGQAx8YeOObxdsqH6ZGDGg0CkEpRAGhRQC1J2Gxm179sxeUr6iqKK81L7xJ6RhgKtjCJ8+CzAGEijAWMlc88rJxhXru1vPkKMLn6l69viBQw7LhIjqEJRCGvfAglJwxmH7/ICuAz29sYwohX7tBqy+gcR+8bw8BHPy9IO/NBU4rUhkMDo4DJ9bgRwHx4FxgDAoyEAwkYk0KOzRUAoUAExJoA4N0ybt7hzl+EhP18nV75q5JbOeckHGVh3VIeJe6wakbiR908G1KIzr7ZCMJaARbzr+Nliotr1lb5DT9sTmfVH29IeVb73yUXamDzAoZHzlhnEvg/En7ewCC/2bgIZsoNvkI3WjXXW3LO0cgIskuUEOzKvcunjVyu1TstIJ9CRbksB2cDDFgojDidsROrx75HbdLUs7L6VsAQBHskfV18/tbPv+xxrdYuBaNFURDWyCr1JRMOpN1yZC7wPXF5R5i4umP+2wLAgtmiQNIqzC6u0DISQRTzhHVma6f1V2/sPJ0EQdA0BjoMybPrOoeeqiR5cqoVHIcZ+laUHqUbDgXZB4ZwkJIQQEF+BOF8Jur+waurupur99bwr450CZx19ceDqndOaTLiMKaRiAZYHYNqDrEKMhEClBCCAlIIUAFwKccXDGwSRByGCyh5mb1ozDSXNgocdXFGjKmlFY4bZNCMMAsSwQxkAMHVBVKA4HiIMAiJGFEOA8BmY2A7MZbMah2pC9zNxU1de+1+lyKWuypj9S4TKi4AaFNE0onAGaBkIpnC4nFKcCJd0PQU2Acwgu4OD8nt/j9kxyClJA3B8vz8hpcCqcn4n29kf9fo+fUApiWZBjKhQCON0uuNxOOOaWQs1+yPRnZ6a5fj0LGY6AMJI4doWI/YgxYITbnU1aKF/5KjQw/Pu2mgsuOF50aLqLqGE4HSQBlYEAgpSpq/d/st3X3ddZuGRpuTuiEgfjiSNScAHdZOi3zI51A527dClaElVRH5hbmau4T6X73H6fzwNXmgtycg6GOMLVzQ27rtJYndbPf2LL7HmlOzNDw0RQCpNaUMMR9Eb1jjV9nbXD3D4tpQymdF5jYQyeMcnvVzwejHIZeu/6xT1thnYuuU5/ml22uSg3r3aSQxB1JIw7mtbx5j+dtUPj0IRHyToVKK1sLX5s7Pz0+UPl3owtAB5/0PV0IlC6sW3GAt4QKL2Zq7heBzD1f6+muNZNzp+S4VAqAMyP1/qD9FzG5FkAlgHImzj3HydF2TVawiXUAAAAAElFTkSuQmCC);
					float:left;
					margin-right: 10px;
				}	
				.footer {
					margin: 10px;
					font-size: 12px;
					float:right;
				}	
				</STYLE>
			</HEAD>
			<BODY BGCOLOR="#FFFFFF">
				<div style="height: 128px">
					<div class="logo"></div>
					<h1>Nnodes</h1>
					<h2>records status</h2>
				</div>
				<div style="clear:both"/>
				<br/>
				<table cellspacing="0">
					<xsl:apply-templates/>
				</table>
				<div class="footer">Nnodes Ver. <xsl:value-of select="@version"/></div>
				</BODY>
		</HTML>
	</xsl:template>
	<xsl:template match="record">
		<tr class="record">
  			<td width="30" style="">
				<xsl:if test="@unavailable = '0'">
	  				<div class="ok"/>
				</xsl:if>
	  			<xsl:if test="@unavailable > '0'">
	  				<div class="ko"/>
				</xsl:if>
  			</td>
			<td><xsl:value-of select="@name"/></td>
			<td><xsl:value-of select="@class"/></td>
			<td><xsl:value-of select="@type"/></td>
			<td><xsl:value-of select="text()"/></td>
		</tr>
	</xsl:template>
</xsl:stylesheet>