select webay_order.order_id, webay_commodity.name as goods, 
webay_commodity.price, webay_order_form.quantity as 'count', 
webay_order_form.sum_of_money as 'total cost', webay_order.date_time 
from webay_user inner join webay_commodity inner join webay_order 
inner join webay_order_form 
on webay_user.user_id = webay_order.user_id 
and webay_order_form.order_id = webay_order.order_id 
and webay_order_form.commodity_id = webay_commodity.commodity_id 
where webay_user.name = 'yang'
